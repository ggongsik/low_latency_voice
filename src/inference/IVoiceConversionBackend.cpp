#include "inference/IVoiceConversionBackend.h"

namespace llvc::inference {

common::Result IVoiceConversionBackend::warmUp(const common::AudioChunk& input,
                                               std::size_t iterations) {
  common::AudioChunk output;
  const auto runCount = iterations == 0 ? std::size_t{1} : iterations;
  for (std::size_t iteration = 0; iteration < runCount; ++iteration) {
    const auto result = process(input, output);
    if (!result) {
      return result;
    }
  }

  return common::Result::ok();
}

} // namespace llvc::inference
