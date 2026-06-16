#include "audio/AudioEngine.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <algorithm>
#include <array>
#include <memory>
#include <utility>

namespace {

class MainComponent final : public juce::AudioAppComponent {
public:
  MainComponent() {
    settings_.sampleRate = 48000.0;
    settings_.blockSize = 128;
    settings_.inputChannels = 2;
    settings_.outputChannels = 2;
    settings_.bypass = true;
    engine_.configure(settings_);

    setSize(420, 160);
    setAudioChannels(static_cast<int>(settings_.inputChannels),
                     static_cast<int>(settings_.outputChannels));
  }

  ~MainComponent() override { shutdownAudio(); }

  void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
    settings_.sampleRate = sampleRate;
    settings_.blockSize = static_cast<std::size_t>(samplesPerBlockExpected);
    engine_.configure(settings_);
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

    engine_.processPassThrough(inputs.data(), outputs.data(), boundedChannels, boundedChannels, frames);
  }

  void releaseResources() override {}

  void paint(juce::Graphics& graphics) override {
    graphics.fillAll(juce::Colours::black);
    graphics.setColour(juce::Colours::white);
    graphics.setFont(16.0F);
    graphics.drawFittedText("Low Latency Voice Changer",
                            getLocalBounds().reduced(24).removeFromTop(32),
                            juce::Justification::centredLeft, 1);

    graphics.setFont(13.0F);
    const auto estimatedBufferMs =
        1000.0 * static_cast<double>(settings_.blockSize) / settings_.sampleRate;
    graphics.drawFittedText("Pass-through skeleton | buffer " +
                                juce::String(estimatedBufferMs, 2) + " ms | blocks " +
                                juce::String(static_cast<juce::int64>(engine_.processedBlocks())),
                            getLocalBounds().reduced(24).withTrimmedTop(48),
                            juce::Justification::centredLeft, 2);
  }

private:
  llvc::audio::AudioSettings settings_{};
  llvc::audio::AudioEngine engine_{};
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
