#include <jni.h>

#include <cstdint>  // for uint64_t
#include <memory>
#include <unordered_set>
#include <vector>  // for allocator, vector

#include "filter/minimal-taffy-cuckoo.hpp"
#include "filter/taffy-block.hpp"
#include "filter/taffy-cuckoo.hpp"
#include "gtest/gtest.h"
#include "util.hpp"  // for Rand
#if defined(__x86_64)
#include "filter/taffy-vector-quotient.hpp"
#endif

#include "filter/block.hpp"

using namespace filter;
using namespace std;

template <typename F>
class BlockTest : public ::testing::Test {};

template <typename F>
class UnionTest : public ::testing::Test {};

template <typename F>
class BytesTest : public ::testing::Test {};

template <typename F>
class NdvFppTest : public ::testing::Test {};

using BlockTypes = ::testing::Types<BlockFilter, ScalarBlockFilter>;
using CreatedWithBytes = ::testing::Types<TaffyCuckooFilter, MinimalTaffyCuckooFilter,
                                          BlockFilter, ScalarBlockFilter>;
using CreatedWithNdvFpp = ::testing::Types<TaffyBlockFilter>;
using UnionTypes = ::testing::Types<TaffyCuckooFilter>;

TYPED_TEST_SUITE(BlockTest, BlockTypes);
TYPED_TEST_SUITE(BytesTest, CreatedWithBytes);
TYPED_TEST_SUITE(NdvFppTest, CreatedWithNdvFpp);
TYPED_TEST_SUITE(UnionTest, UnionTypes);
// TODO: test hidden methods in libfilter.so

// TODO: test more methods, including copy

TYPED_TEST(UnionTest, UnionDoes) {
  for (unsigned xndv = 1; xndv < 200; ++xndv) {
    for (unsigned yndv = 1; yndv < 1024; yndv += xndv) {
      // cout << "ndv " << dec << xndv << " " << yndv << endl;
      Rand r;
      vector<uint64_t> xhashes, yhashes;
      auto x = TypeParam::CreateWithBytes(0);
      auto y = TypeParam::CreateWithBytes(0);
      for (unsigned i = 0; i < xndv; ++i) {
        xhashes.push_back(r());
        x.InsertHash(xhashes.back());
      }
      for (unsigned i = 0; i < yndv; ++i) {
        yhashes.push_back(r());
        y.InsertHash(yhashes.back());
      }
      //cout << "xy " << dec << xndv << " " << yndv << endl;
      auto z = Union(x, y);
      for (unsigned j = 0; j < xhashes.size(); ++j) {
        //cout << "x " << dec << xndv << " " << yndv << " " << j << " " << hex << "0x"
        //   << xhashes[j] << endl;
        EXPECT_TRUE(z.FindHash(xhashes[j]))
            << xndv << " " << yndv << " " << j << " " << hex << "0x" << xhashes[j];
      }
      for (unsigned j = 0; j < yhashes.size(); ++j) {
        //cout << "y " << dec << xndv << " " << yndv << " " << j << " " << hex << "0x"
        //   << yhashes[j] << endl;
        EXPECT_TRUE(z.FindHash(yhashes[j]))
            << xndv << " " << yndv << " " << j << " " << hex << "0x" << yhashes[j];
      }
    }
  }
}

TYPED_TEST(UnionTest, UnionFpp) {
  Rand r;
  vector<uint64_t> missing;
  for (unsigned absent = 1; absent < (1 << 18); ++absent) {
    missing.push_back(r());
  }

  for (unsigned xndv = 1; xndv < 1024; xndv *= 2) {
    for (unsigned yndv = 1; yndv < 1024; yndv *= 2) {
      auto x = TypeParam::CreateWithBytes(0);
      auto y = TypeParam::CreateWithBytes(0);
      for (unsigned i = 0; i < xndv; ++i) {
        x.InsertHash(r());
      }
      for (unsigned i = 0; i < yndv; ++i) {
        y.InsertHash(r());
      }
      //cout << "xy " << dec << xndv << " " << yndv << endl;
      auto z = Union(x, y);
      for (auto v : missing) {
        EXPECT_EQ(z.FindHash(v), x.FindHash(v) || y.FindHash(v)) << xndv << " " << yndv << " " << v;
      }
    }
  }
}

template <typename T>
void InsertPersistsHelp(T& x, vector<uint64_t>& hashes) {
  Rand r;

  for (unsigned i = 0; i < hashes.size(); ++i) {
    hashes[i] = r();
  }
  for (unsigned i = 0; i < hashes.size(); ++i) {
    x.InsertHash(hashes[i]);
    for (unsigned j = 0; j <= i; ++j) {
      EXPECT_TRUE(x.FindHash(hashes[j]))
          << dec << j << " of " << i << " of " << hashes.size() << " with hash 0x" << hex
          << hashes[j];
      if (not x.FindHash(hashes[j])) {
        throw 2;
      }
    }
  }
}

template <typename T>
void InsertFalsePositivePersistsHelp(T& x, uint64_t n) {
  Rand r;
  vector<uint64_t> seen;

  for (uint64_t i = 0; i < n; ++i) {
    x.InsertHash(r());
    auto y = r();
    if (x.FindHash(y)) {
      seen.push_back(y);
      // std::cout << std::hex << y << " of " << std::dec << seen.size() << " of " << i
      //           << std::endl;
    }
    for (uint64_t j = 0; j < seen.size(); ++j) {
      EXPECT_TRUE(x.FindHash(seen[j])) << dec << j << " of " << seen.size() << " of " << i
                                       << " with hash 0x" << hex << seen[j];
      if (not x.FindHash(seen[j])) {
        throw 2;
      }
    }
  }
}

// Test that once something is inserted, it's always present
TYPED_TEST(BytesTest, InsertPersistsWithBytes) {
  auto ndv = 16000;
  auto x = TypeParam::CreateWithBytes(ndv);
  vector<uint64_t> hashes(ndv);
  InsertPersistsHelp(x, hashes);
}

// Test that once something is inserted, it's always present
TYPED_TEST(BytesTest, InsertFalsePositivePersistsWithBytes) {
  auto ndv = 16000;
  auto x = TypeParam::CreateWithBytes(1);
  InsertFalsePositivePersistsHelp(x, ndv);
}

// Test that once something is inserted, it's always present
TYPED_TEST(NdvFppTest, InsertPersistsWithNdvFpp) {
  auto ndv = 16000;
  auto x = TypeParam::CreateWithNdvFpp(ndv, 0.01);
  vector<uint64_t> hashes(ndv);
  InsertPersistsHelp(x, hashes);
}

template<typename T>
void StartEmptyHelp(const T& x, uint64_t ndv) {
  Rand r;
  for (uint64_t j = 0; j < ndv; ++j) {
    auto v = r();
    EXPECT_FALSE(x.FindHash(v)) << v;
  }
}

// Test that filters start with a 0.0 fpp.
TYPED_TEST(BytesTest, StartEmpty) {
  auto ndv = 16000000;
  auto x = TypeParam::CreateWithBytes(ndv);
  return StartEmptyHelp(x, ndv);
}

// Test that filters start with a 0.0 fpp.
TYPED_TEST(NdvFppTest, StartEmpty) {
  auto ndv = 16000000;
  auto fpp = 0.01;
  auto x = TypeParam::CreateWithNdvFpp(ndv, fpp);
  return StartEmptyHelp(x, ndv);
}

// Test that Scalar and Simd variants are identical
TYPED_TEST(BlockTest, Buddy) {
  if (TypeParam::is_simd) {
    auto ndv = 1600000;
    auto x = TypeParam::CreateWithBytes(ndv);
    auto y = TypeParam::Scalar::CreateWithBytes(ndv);
    Rand r;
    for (int i = 0; i < ndv; ++i) {
      auto v = r();
      x.InsertHash(v);
      y.InsertHash(v);
    }
    for (int i = 0; i < ndv; ++i) {
      auto v = r();
      EXPECT_EQ(x.FindHash(v), y.FindHash(v));
    }
  }
}

// Test eqaulity operator
TYPED_TEST(BlockTest, EqualStayEqual) {
  auto ndv = 160000;
  auto x = TypeParam::CreateWithBytes(ndv);
  auto y = TypeParam::CreateWithBytes(ndv);
  vector<uint64_t> hashes(ndv);
  Rand r;
  for (int i = 0; i < ndv; ++i) {
    hashes[i] = r();
  }
  for (int i = 0; i < ndv; ++i) {
    x.InsertHash(hashes[i]);
    y.InsertHash(hashes[i]);
    auto z = x;
    EXPECT_TRUE(x == y);
    EXPECT_TRUE(y == z);
    EXPECT_TRUE(z == x);
  }
}

TEST(FreezeTest, FreezeTest) {
  Rand r;
  vector<uint64_t> keys;
  TaffyCuckooFilter tcf = TaffyCuckooFilter::CreateWithBytes(0);
  for (size_t i = 0; i < 5000000; ++i) {
    keys.push_back(r());
    tcf.InsertHash(keys.back());
  }
  auto ftcf = tcf.Freeze();
  for (size_t i = 0; i < keys.size(); ++i) {
    EXPECT_TRUE(ftcf.FindHash(keys[i]));
  }
}

TEST(SerDeTest, SerDeTest) {
  Rand r;
  for (size_t size = 1; size < 1 << 20; size *= 2) {
    BlockFilter f = BlockFilter::CreateWithNdvFpp(size, 0.01);
    for (size_t i = 0; i < size; ++i) f.InsertHash(r());
    vector<char> serialized(f.SizeInBytes());
    f.Serialize(serialized.data());
    auto g = BlockFilter::Deserialize(f.SizeInBytes(), serialized.data());
    EXPECT_TRUE(f == g) << size << ", " << f.SizeInBytes() << ", " << g.SizeInBytes();
  }
}

TEST(SerDeTest, JavaSerDeTest) {
  JavaVM* jvm = nullptr;
  JNIEnv* env = nullptr;
  JavaVMInitArgs vm_args;
  unique_ptr<JavaVMOption[]> options{new JavaVMOption[1]};
  auto classpath = string(
      "-Djava.class.path=./java/libfilter/target/"
      "libfilter-0.3.0-SNAPSHOT-jar-with-dependencies.jar");
  unique_ptr<char []> classpath_c_str{new char[classpath.size()+1]};
  memcpy(classpath_c_str.get(), classpath.c_str(), classpath.size());
  classpath_c_str[classpath.size()] = 0;
  options[0].optionString = classpath_c_str.get();
  vm_args.version = JNI_VERSION_1_6;
  vm_args.nOptions = 1;
  vm_args.options = options.get();
  jint rc = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
  ASSERT_EQ(rc, JNI_OK);
  jclass BlockFilter = env->FindClass("com/github/jbapple/libfilter/BlockFilter");
    ASSERT_NE(BlockFilter, nullptr);
  jmethodID CreateWithNdvFpp = env->GetStaticMethodID(
      BlockFilter, "CreateWithNdvFpp", "(DD)Lcom/github/jbapple/libfilter/BlockFilter;");
  ASSERT_NE(CreateWithNdvFpp, nullptr);
  jobject filter = env->CallStaticObjectMethod(BlockFilter, CreateWithNdvFpp,
                                               static_cast<jdouble>(1000) * 1000, 0.01);
  ASSERT_NE(filter, nullptr);
  auto AddHash64 = env->GetMethodID(BlockFilter, "AddHash64", "(J)Z");
  ASSERT_NE(AddHash64, nullptr);
  env->CallBooleanMethod(filter, AddHash64, 867 + 5309);
  auto GetPayload = env->GetMethodID(BlockFilter, "getPayload", "()[I");
  ASSERT_NE(GetPayload, nullptr);
  auto payload = (jintArray)env->CallObjectMethod(filter, GetPayload);
  ASSERT_NE(payload, nullptr);

  jsize size_in_ints = env->GetArrayLength(payload);
  jint* raw_payload = env->GetIntArrayElements(payload, 0);
  auto cpp_filter = BlockFilter::DeserializeFromInts(size_in_ints, raw_payload);

  auto cpp_filter2 = BlockFilter::CreateWithNdvFpp(1000 * 1000, 0.01);
  cpp_filter2.InsertHash(867 + 5309);
  EXPECT_TRUE(cpp_filter == cpp_filter2)
      << cpp_filter.SizeInBytes() << " " << cpp_filter2.SizeInBytes();
  env->ReleaseIntArrayElements(payload, raw_payload, 0);
  jvm->DestroyJavaVM();
}
