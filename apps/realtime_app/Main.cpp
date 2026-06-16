#include "audio/AudioEngine.h"
#include "audio/AudioWorkerPipeline.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <algorithm>
#include <array>
#include <memory>
#include <utility>

namespace {

class MainComponent final : public juce::AudioAppComponent,
                            private juce::Button::Listener,
                            private juce::Timer {
public:
  MainComponent() {
    llvc::audio::AudioSettings initialSettings;
    initialSettings.sampleRate = 48000.0;
    initialSettings.blockSize = 128;
    initialSettings.inputChannels = 2;
    initialSettings.outputChannels = 2;
    initialSettings.bypass = true;
    engine_.configure(initialSettings);
    workerPipeline_.start();

    setAudioChannels(static_cast<int>(initialSettings.inputChannels),
                     static_cast<int>(initialSettings.outputChannels));

    titleLabel_.setText("Low Latency Voice Changer", juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(18.0F, juce::Font::bold));
    titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel_);

    statusLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    statusLabel_.setFont(juce::Font(13.0F));
    statusLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel_);

    bypassButton_.setButtonText("Bypass");
    bypassButton_.setClickingTogglesState(true);
    bypassButton_.setToggleState(true, juce::dontSendNotification);
    bypassButton_.addListener(this);
    addAndMakeVisible(bypassButton_);

    resetCountersButton_.setButtonText("Reset");
    resetCountersButton_.addListener(this);
    addAndMakeVisible(resetCountersButton_);

    deviceSelector_ = std::make_unique<juce::AudioDeviceSelectorComponent>(
        deviceManager, 1, 2, 1, 2, false, false, true, false);
    deviceSelector_->setName("Audio Device Settings");
    addAndMakeVisible(*deviceSelector_);

    setSize(720, 520);
    updateStatusLabel();
    startTimerHz(8);
  }

  ~MainComponent() override {
    stopTimer();
    bypassButton_.removeListener(this);
    resetCountersButton_.removeListener(this);
    shutdownAudio();
    workerPipeline_.stop();
  }

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
    auto currentSettings = engine_.settings();
    currentSettings.sampleRate = sampleRate;
    currentSettings.blockSize = static_cast<std::size_t>(samplesPerBlockExpected);
    engine_.configure(currentSettings);
  }

  void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override {
    auto* buffer = bufferToFill.buffer;
    if (buffer == nullptr) {
      engine_.noteXrun();
      return;
    }

    const auto channels = static_cast<std::size_t>(buffer->getNumChannels());
    const auto frames = static_cast<std::size_t>(bufferToFill.numSamples);
    std::array<const float*, 16> inputs{};
    std::array<float*, 16> outputs{};
    const auto boundedChannels = std::min<std::size_t>(channels, inputs.size());

    for (std::size_t channel = 0; channel < boundedChannels; ++channel) {
      inputs[channel] = buffer->getReadPointer(static_cast<int>(channel), bufferToFill.startSample);
      outputs[channel] = buffer->getWritePointer(static_cast<int>(channel), bufferToFill.startSample);
    }

    (void)workerPipeline_.processShadowFromAudioThread(inputs.data(), boundedChannels, frames);
    engine_.processPassThrough(inputs.data(), outputs.data(), boundedChannels, boundedChannels, frames);
  }

  void releaseResources() override {}

  void paint(juce::Graphics& graphics) override {
    graphics.fillAll(juce::Colour(0xff111315));
    graphics.setColour(juce::Colour(0xff2a3036));
    graphics.drawHorizontalLine(72, 24.0F, static_cast<float>(getWidth() - 24));
  }

  void resized() override {
    auto bounds = getLocalBounds().reduced(24);
    auto header = bounds.removeFromTop(40);
    titleLabel_.setBounds(header.removeFromLeft(360));
    resetCountersButton_.setBounds(header.removeFromRight(88).reduced(0, 4));
    bypassButton_.setBounds(header.removeFromRight(112).reduced(0, 4));

    statusLabel_.setBounds(bounds.removeFromTop(32));
    bounds.removeFromTop(16);

    if (deviceSelector_ != nullptr) {
      deviceSelector_->setBounds(bounds);
    }
  }

private:
  void buttonClicked(juce::Button* button) override {
    if (button == &bypassButton_) {
      engine_.setBypass(bypassButton_.getToggleState());
      updateStatusLabel();
    } else if (button == &resetCountersButton_) {
      engine_.resetCounters();
      workerPipeline_.resetCounters();
      updateStatusLabel();
    }
  }

  void timerCallback() override {
    updateStatusLabel();
    repaint();
  }

  void updateStatusLabel() {
    const auto workerStats = workerPipeline_.stats();
    statusLabel_.setText(
        "buffer " + juce::String(engine_.blockSize()) + " @ " +
            juce::String(engine_.sampleRate(), 0) + " Hz | " +
            juce::String(engine_.estimatedBlockLatencyMs(), 2) + " ms | blocks " +
            juce::String(static_cast<juce::int64>(engine_.processedBlocks())) + " | xruns " +
            juce::String(static_cast<juce::int64>(engine_.xrunCount())) + " | worker q " +
            juce::String(static_cast<int>(workerStats.inputQueueSize)) + "/" +
            juce::String(static_cast<int>(workerStats.outputQueueSize)) + " | late " +
            juce::String(static_cast<juce::int64>(workerStats.lateOutputBlocks)),
        juce::dontSendNotification);
  }

  llvc::audio::AudioEngine engine_{};
  llvc::audio::AudioWorkerPipeline workerPipeline_{};
  juce::Label titleLabel_;
  juce::Label statusLabel_;
  juce::ToggleButton bypassButton_;
  juce::TextButton resetCountersButton_;
  std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector_;
};

class RealtimeVoiceChangerApplication final : public juce::JUCEApplication {
public:
  const juce::String getApplicationName() override { return "Low Latency Voice Changer"; }
  const juce::String getApplicationVersion() override { return "0.1.0"; }
  bool moreThanOneInstanceAllowed() override { return true; }

  void initialise(const juce::String&) override {
    mainWindow_ = std::make_unique<MainWindow>(getApplicationName());
  }

  void shutdown() override { mainWindow_ = nullptr; }
  void systemRequestedQuit() override { quit(); }
  void anotherInstanceStarted(const juce::String&) override {}

private:
  class MainWindow final : public juce::DocumentWindow {
  public:
    explicit MainWindow(juce::String name)
        : DocumentWindow(std::move(name), juce::Colours::black,
                         DocumentWindow::allButtons) {
      setUsingNativeTitleBar(true);
      setContentOwned(new MainComponent(), true);
      centreWithSize(getWidth(), getHeight());
      setVisible(true);
    }

    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
  };

  std::unique_ptr<MainWindow> mainWindow_;
};

} // namespace

START_JUCE_APPLICATION(RealtimeVoiceChangerApplication)
