// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdint.h>
#include <stdio.h>

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "lib/extras/codec.h"
#include "lib/jxl/aux_out.h"
#include "lib/jxl/base/compiler_specific.h"
#include "lib/jxl/base/data_parallel.h"
#include "lib/jxl/base/override.h"
#include "lib/jxl/base/padded_bytes.h"
#include "lib/jxl/base/thread_pool_internal.h"
#include "lib/jxl/codec_in_out.h"
#include "lib/jxl/color_encoding_internal.h"
#include "lib/jxl/color_management.h"
#include "lib/jxl/dec_file.h"
#include "lib/jxl/dec_params.h"
#include "lib/jxl/enc_butteraugli_comparator.h"
#include "lib/jxl/enc_butteraugli_pnorm.h"
#include "lib/jxl/enc_cache.h"
#include "lib/jxl/enc_color_management.h"
#include "lib/jxl/enc_file.h"
#include "lib/jxl/enc_params.h"
#include "lib/jxl/image.h"
#include "lib/jxl/image_bundle.h"
#include "lib/jxl/image_ops.h"
#include "lib/jxl/image_test_utils.h"
#include "lib/jxl/modular/encoding/enc_encoding.h"
#include "lib/jxl/modular/encoding/encoding.h"
#include "lib/jxl/test_utils.h"
#include "lib/jxl/testdata.h"

namespace jxl {
namespace {
using test::Roundtrip;

void TestLosslessGroups(size_t group_size_shift) {
  ThreadPool* pool = nullptr;
  const PaddedBytes orig =
      ReadTestData("imagecompression.info/flower_foveon.png");
  CompressParams cparams;
  cparams.SetLossless();
  cparams.modular_group_size_shift = group_size_shift;
  DecompressParams dparams;

  CodecInOut io_out;
  size_t compressed_size;

  CodecInOut io;
  ASSERT_TRUE(SetFromBytes(Span<const uint8_t>(orig), &io, pool));
  io.ShrinkTo(io.xsize() / 4, io.ysize() / 4);

  compressed_size = Roundtrip(&io, cparams, dparams, pool, &io_out);
  EXPECT_LE(compressed_size, 280000u);
  EXPECT_LE(ButteraugliDistance(io, io_out, cparams.ba_params, GetJxlCms(),
                                /*distmap=*/nullptr, pool),
            0.0);
}

TEST(ModularTest, RoundtripLosslessGroups128) { TestLosslessGroups(0); }

TEST(ModularTest, JXL_TSAN_SLOW_TEST(RoundtripLosslessGroups512)) {
  TestLosslessGroups(2);
}

TEST(ModularTest, JXL_TSAN_SLOW_TEST(RoundtripLosslessGroups1024)) {
  TestLosslessGroups(3);
}

TEST(ModularTest, RoundtripLosslessCustomWP_PermuteRCT) {
  ThreadPool* pool = nullptr;
  const PaddedBytes orig =
      ReadTestData("wesaturate/500px/u76c0g_bliznaca_srgb8.png");
  CompressParams cparams;
  cparams.SetLossless();
  // 9 = permute to GBR, to test the special case of permutation-only
  cparams.colorspace = 9;
  // slowest speed so different WP modes are tried
  cparams.speed_tier = SpeedTier::kTortoise;
  cparams.options.predictor = {Predictor::Weighted};
  DecompressParams dparams;

  CodecInOut io_out;
  size_t compressed_size;

  CodecInOut io;
  ASSERT_TRUE(SetFromBytes(Span<const uint8_t>(orig), &io, pool));
  io.ShrinkTo(100, 100);

  compressed_size = Roundtrip(&io, cparams, dparams, pool, &io_out);
  EXPECT_LE(compressed_size, 10150u);
  EXPECT_LE(ButteraugliDistance(io, io_out, cparams.ba_params, GetJxlCms(),
                                /*distmap=*/nullptr, pool),
            0.0);
}

TEST(ModularTest, RoundtripLossyDeltaPalette) {
  ThreadPool* pool = nullptr;
  const PaddedBytes orig =
      ReadTestData("wesaturate/500px/u76c0g_bliznaca_srgb8.png");
  CompressParams cparams;
  cparams.modular_mode = true;
  cparams.color_transform = jxl::ColorTransform::kNone;
  cparams.lossy_palette = true;
  cparams.palette_colors = 0;

  DecompressParams dparams;

  CodecInOut io_out;
  size_t compressed_size;

  CodecInOut io;
  ASSERT_TRUE(SetFromBytes(Span<const uint8_t>(orig), &io, pool));
  io.ShrinkTo(300, 100);

  compressed_size = Roundtrip(&io, cparams, dparams, pool, &io_out);
  EXPECT_LE(compressed_size, 6800u);
  cparams.ba_params.intensity_target = 80.0f;
  EXPECT_THAT(ButteraugliDistance(io, io_out, cparams.ba_params, GetJxlCms(),
                                  /*distmap=*/nullptr, pool),
              IsSlightlyBelow(1.5));
}
TEST(ModularTest, RoundtripLossyDeltaPaletteWP) {
  ThreadPool* pool = nullptr;
  const PaddedBytes orig =
      ReadTestData("wesaturate/500px/u76c0g_bliznaca_srgb8.png");
  CompressParams cparams;
  cparams.SetLossless();
  cparams.lossy_palette = true;
  cparams.palette_colors = 0;
  cparams.options.predictor = jxl::Predictor::Weighted;

  DecompressParams dparams;

  CodecInOut io_out;
  size_t compressed_size;

  CodecInOut io;
  ASSERT_TRUE(SetFromBytes(Span<const uint8_t>(orig), &io, pool));
  io.ShrinkTo(300, 100);

  compressed_size = Roundtrip(&io, cparams, dparams, pool, &io_out);
  EXPECT_LE(compressed_size, 7000u);
  cparams.ba_params.intensity_target = 80.0f;
  EXPECT_THAT(ButteraugliDistance(io, io_out, cparams.ba_params, GetJxlCms(),
                                  /*distmap=*/nullptr, pool),
              IsSlightlyBelow(10.0));
}

TEST(ModularTest, RoundtripLossy) {
  ThreadPool* pool = nullptr;
  const PaddedBytes orig =
      ReadTestData("wesaturate/500px/u76c0g_bliznaca_srgb8.png");
  CompressParams cparams;
  cparams.modular_mode = true;
  cparams.butteraugli_distance = 2.f;
  DecompressParams dparams;

  CodecInOut io_out;
  size_t compressed_size;

  CodecInOut io;
  ASSERT_TRUE(SetFromBytes(Span<const uint8_t>(orig), &io, pool));

  compressed_size = Roundtrip(&io, cparams, dparams, pool, &io_out);
  EXPECT_LE(compressed_size, 30000u);
  cparams.ba_params.intensity_target = 80.0f;
  EXPECT_THAT(ButteraugliDistance(io, io_out, cparams.ba_params, GetJxlCms(),
                                  /*distmap=*/nullptr, pool),
              IsSlightlyBelow(2.3));
}

TEST(ModularTest, RoundtripLossy16) {
  ThreadPool* pool = nullptr;
  const PaddedBytes orig =
      ReadTestData("raw.pixls/DJI-FC6310-16bit_709_v4_krita.png");
  CompressParams cparams;
  cparams.modular_mode = true;
  cparams.butteraugli_distance = 2.f;
  DecompressParams dparams;

  CodecInOut io_out;
  size_t compressed_size;

  CodecInOut io;
  ASSERT_TRUE(SetFromBytes(Span<const uint8_t>(orig), &io, pool));
  JXL_CHECK(io.TransformTo(ColorEncoding::SRGB(), GetJxlCms(), pool));
  io.metadata.m.color_encoding = ColorEncoding::SRGB();

  compressed_size = Roundtrip(&io, cparams, dparams, pool, &io_out);
  EXPECT_LE(compressed_size, 300u);
  cparams.ba_params.intensity_target = 80.0f;
  EXPECT_THAT(ButteraugliDistance(io, io_out, cparams.ba_params, GetJxlCms(),
                                  /*distmap=*/nullptr, pool),
              IsSlightlyBelow(1.6));
}

TEST(ModularTest, RoundtripExtraProperties) {
  constexpr size_t kSize = 250;
  Image image(kSize, kSize, /*bitdepth=*/8, 3);
  ModularOptions options;
  options.max_properties = 4;
  options.predictor = Predictor::Zero;
  Rng rng(0);
  for (size_t y = 0; y < kSize; y++) {
    for (size_t x = 0; x < kSize; x++) {
      image.channel[0].plane.Row(y)[x] = image.channel[2].plane.Row(y)[x] =
          rng.UniformU(0, 9);
    }
  }
  ZeroFillImage(&image.channel[1].plane);
  BitWriter writer;
  ASSERT_TRUE(ModularGenericCompress(image, options, &writer));
  writer.ZeroPadToByte();
  Image decoded(kSize, kSize, /*bitdepth=*/8, image.channel.size());
  for (size_t i = 0; i < image.channel.size(); i++) {
    const Channel& ch = image.channel[i];
    decoded.channel[i] = Channel(ch.w, ch.h, ch.hshift, ch.vshift);
  }
  Status status = true;
  {
    BitReader reader(writer.GetSpan());
    BitReaderScopedCloser closer(&reader, &status);
    ASSERT_TRUE(ModularGenericDecompress(&reader, decoded, /*header=*/nullptr,
                                         /*group_id=*/0, &options));
  }
  ASSERT_TRUE(status);
  ASSERT_EQ(image.channel.size(), decoded.channel.size());
  for (size_t c = 0; c < image.channel.size(); c++) {
    for (size_t y = 0; y < image.channel[c].plane.ysize(); y++) {
      for (size_t x = 0; x < image.channel[c].plane.xsize(); x++) {
        EXPECT_EQ(image.channel[c].plane.Row(y)[x],
                  decoded.channel[c].plane.Row(y)[x])
            << "c = " << c << ", x = " << x << ",  y = " << y;
      }
    }
  }
}

TEST(ModularTest, RoundtripLosslessCustomSqueeze) {
  ThreadPool* pool = nullptr;
  const PaddedBytes orig =
      ReadTestData("wesaturate/500px/tmshre_riaphotographs_srgb8.png");
  CodecInOut io;
  ASSERT_TRUE(SetFromBytes(Span<const uint8_t>(orig), &io, pool));

  CompressParams cparams;
  cparams.modular_mode = true;
  cparams.color_transform = jxl::ColorTransform::kNone;
  cparams.butteraugli_distance = 0.f;
  cparams.options.predictor = {Predictor::Zero};
  cparams.speed_tier = SpeedTier::kThunder;
  cparams.responsive = 1;
  // Custom squeeze params, atm just for testing
  SqueezeParams p;
  p.horizontal = true;
  p.in_place = false;
  p.begin_c = 0;
  p.num_c = 3;
  cparams.squeezes.push_back(p);
  p.begin_c = 1;
  p.in_place = true;
  p.horizontal = false;
  cparams.squeezes.push_back(p);
  DecompressParams dparams;

  CodecInOut io2;
  EXPECT_LE(Roundtrip(&io, cparams, dparams, pool, &io2), 265000u);
  EXPECT_EQ(0.0, ButteraugliDistance(io, io2, cparams.ba_params, GetJxlCms(),
                                     /*distmap=*/nullptr, pool));
}

struct RoundtripLosslessConfig {
  int bitdepth;
  int responsive;
};
class ModularTestParam
    : public ::testing::TestWithParam<RoundtripLosslessConfig> {};

std::vector<RoundtripLosslessConfig> GenerateLosslessTests() {
  std::vector<RoundtripLosslessConfig> all;
  for (int responsive = 0; responsive <= 1; responsive++) {
    for (int bitdepth = 1; bitdepth < 32; bitdepth++) {
      if (responsive && bitdepth > 30) continue;
      all.push_back({bitdepth, responsive});
    }
  }
  return all;
}
std::string LosslessTestDescription(
    const testing::TestParamInfo<ModularTestParam::ParamType>& info) {
  std::stringstream name;
  name << info.param.bitdepth << "bit";
  if (info.param.responsive) name << "Squeeze";
  return name.str();
}

JXL_GTEST_INSTANTIATE_TEST_SUITE_P(RoundtripLossless, ModularTestParam,
                                   testing::ValuesIn(GenerateLosslessTests()),
                                   LosslessTestDescription);

TEST_P(ModularTestParam, RoundtripLossless) {
  RoundtripLosslessConfig config = GetParam();
  int bitdepth = config.bitdepth;
  int responsive = config.responsive;

  ThreadPool* pool = nullptr;
  Rng generator(123);
  const PaddedBytes orig =
      ReadTestData("wesaturate/500px/u76c0g_bliznaca_srgb8.png");
  CodecInOut io1;
  ASSERT_TRUE(SetFromBytes(Span<const uint8_t>(orig), &io1, pool));

  // vary the dimensions a bit, in case of bugs related to
  // even vs odd width or height.
  size_t xsize = 423 + bitdepth;
  size_t ysize = 467 + bitdepth;

  CodecInOut io;
  io.SetSize(xsize, ysize);
  io.metadata.m.color_encoding = jxl::ColorEncoding::SRGB(false);
  io.metadata.m.SetUintSamples(bitdepth);

  double factor = ((1lu << bitdepth) - 1lu);
  double ifactor = 1.0 / factor;
  Image3F noise_added(xsize, ysize);

  for (size_t c = 0; c < 3; c++) {
    for (size_t y = 0; y < ysize; y++) {
      const float* in = io1.Main().color()->PlaneRow(c, y);
      float* out = noise_added.PlaneRow(c, y);
      for (size_t x = 0; x < xsize; x++) {
        // make the least significant bits random
        float f = in[x] + generator.UniformF(0.0f, 1.f / 255.f);
        if (f > 1.f) f = 1.f;
        // quantize to the bitdepth we're testing
        unsigned int u = f * factor + 0.5;
        out[x] = u * ifactor;
      }
    }
  }
  io.SetFromImage(std::move(noise_added), jxl::ColorEncoding::SRGB(false));

  CompressParams cparams;
  cparams.modular_mode = true;
  cparams.color_transform = jxl::ColorTransform::kNone;
  cparams.butteraugli_distance = 0.f;
  cparams.options.predictor = {Predictor::Zero};
  cparams.speed_tier = SpeedTier::kThunder;
  cparams.responsive = responsive;
  DecompressParams dparams;
  CodecInOut io2;
  EXPECT_LE(Roundtrip(&io, cparams, dparams, pool, &io2),
            bitdepth * xsize * ysize / 3);
  EXPECT_LE(0, ComputeDistance2(io.Main(), io2.Main(), GetJxlCms()));
  size_t different = 0;
  for (size_t c = 0; c < 3; c++) {
    for (size_t y = 0; y < ysize; y++) {
      const float* in = io.Main().color()->PlaneRow(c, y);
      const float* out = io2.Main().color()->PlaneRow(c, y);
      for (size_t x = 0; x < xsize; x++) {
        uint32_t uin = in[x] * factor + 0.5;
        uint32_t uout = out[x] * factor + 0.5;
        // check that the integer values are identical
        if (uin != uout) different++;
      }
    }
  }
  EXPECT_EQ(different, 0);
}

TEST(ModularTest, RoundtripLosslessCustomFloat) {
  ThreadPool* pool = nullptr;
  CodecInOut io;
  size_t xsize = 100, ysize = 300;
  io.SetSize(xsize, ysize);
  io.metadata.m.bit_depth.bits_per_sample = 18;
  io.metadata.m.bit_depth.exponent_bits_per_sample = 6;
  io.metadata.m.bit_depth.floating_point_sample = true;
  io.metadata.m.modular_16_bit_buffer_sufficient = false;
  ColorEncoding color_encoding;
  color_encoding.tf.SetTransferFunction(TransferFunction::kLinear);
  color_encoding.SetColorSpace(ColorSpace::kRGB);
  Image3F testimage(xsize, ysize);
  float factor = 1.f / (1 << 14);
  for (size_t c = 0; c < 3; c++) {
    for (size_t y = 0; y < ysize; y++) {
      float* const JXL_RESTRICT row = testimage.PlaneRow(c, y);
      for (size_t x = 0; x < xsize; x++) {
        row[x] = factor * (x ^ y);
      }
    }
  }
  io.SetFromImage(std::move(testimage), color_encoding);
  io.metadata.m.color_encoding = color_encoding;
  io.metadata.m.SetIntensityTarget(255);

  CompressParams cparams;
  cparams.modular_mode = true;
  cparams.color_transform = jxl::ColorTransform::kNone;
  cparams.butteraugli_distance = 0.f;
  cparams.options.predictor = {Predictor::Zero};
  cparams.speed_tier = SpeedTier::kThunder;
  cparams.decoding_speed_tier = 2;
  DecompressParams dparams;

  CodecInOut io2;
  EXPECT_LE(Roundtrip(&io, cparams, dparams, pool, &io2), 23000u);
  EXPECT_EQ(0.0, ButteraugliDistance(io, io2, cparams.ba_params, GetJxlCms(),
                                     /*distmap=*/nullptr, pool));
}

}  // namespace
}  // namespace jxl
