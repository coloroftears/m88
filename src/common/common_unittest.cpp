// Test skeleton example.

#include "common/clamp.h"
#include "common/types.h"
#include "gtest/gtest.h"

#include <memory>

class BaseTest : public testing::Test {
 public:
  BaseTest() {}
  virtual ~BaseTest() {}

  // Overrides
  void SetUp() {
    buf32.reset(new int32_t[N]);
    buf16.reset(new int16_t[N]);
    for (int i = 0; i < N; ++i) {
      int sign = (i % 2) ? 1 : -1;
      int mul = 2100;
      buf32[i] = sign * (mul * (i * 7 % N));
    }
  }

  void TearDown() {}

 protected:
  const int N = 1000000;
  std::unique_ptr<int32_t[]> buf32;
  std::unique_ptr<int16_t[]> buf16;
};

TEST_F(BaseTest, Basic) {
  for (int i = 0; i < N; ++i)
    buf16[i] = Limit(buf32[i], 32767, -32768);
  for (int i = 0; i < N; ++i)
    EXPECT_EQ(Limit16(buf32[i]), buf16[i]);
}
