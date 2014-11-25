// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device_proximity_event_pump.h"

#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "content/common/device_sensors/device_proximity_hardware_buffer.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebDeviceProximityListener.h"

namespace content {

class MockDeviceProximityListener
    : public blink::WebDeviceProximityListener {
 public:
  MockDeviceProximityListener() : did_change_device_proximity_(false) {
    memset(&data_, 0, sizeof(data_));
  }
  virtual ~MockDeviceProximityListener() { }

  virtual void didChangeDeviceProximity(
      const blink::WebDeviceProximityData& data) override {
    memcpy(&data_, &data, sizeof(data));
    did_change_device_proximity_ = true;
  }

  bool did_change_device_proximity() const {
    return did_change_device_proximity_;
  }
  void set_did_change_device_proximity(bool value) {
    did_change_device_proximity_ = value;
  }
  const blink::WebDeviceProximityData& data() const {
    return data_;
  }

 private:
  bool did_change_device_proximity_;
  blink::WebDeviceProximityData data_;

  DISALLOW_COPY_AND_ASSIGN(MockDeviceProximityListener);
};

class DeviceProximityEventPumpForTesting : public DeviceProximityEventPump {
 public:
  DeviceProximityEventPumpForTesting()
      : DeviceProximityEventPump(0) { }
  ~DeviceProximityEventPumpForTesting() override {}

  void OnDidStart(base::SharedMemoryHandle renderer_handle) {
    DeviceProximityEventPump::OnDidStart(renderer_handle);
  }
  void SendStartMessage() override {}
  void SendStopMessage() override {}
  void FireEvent() override {
    DeviceProximityEventPump::FireEvent();
    Stop();
    base::MessageLoop::current()->QuitWhenIdle();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceProximityEventPumpForTesting);
};

class DeviceProximityEventPumpTest : public testing::Test {
 public:
  DeviceProximityEventPumpTest() {
      EXPECT_TRUE(shared_memory_.CreateAndMapAnonymous(
          sizeof(DeviceProximityHardwareBuffer)));
  }

 protected:
  void SetUp() override {
    const DeviceProximityHardwareBuffer* null_buffer = NULL;
    listener_.reset(new MockDeviceProximityListener);
    proximity_pump_.reset(new DeviceProximityEventPumpForTesting);
    buffer_ = static_cast<DeviceProximityHardwareBuffer*>(
        shared_memory_.memory());
    ASSERT_NE(null_buffer, buffer_);
    memset(buffer_, 0, sizeof(DeviceProximityHardwareBuffer));
    ASSERT_TRUE(shared_memory_.ShareToProcess(base::GetCurrentProcessHandle(),
        &handle_));
  }

  void InitBuffer() {
    blink::WebDeviceProximityData& data = buffer_->data;
    data.value = 1;
    data.min = 1;
    data.max = 1;
  }

  void InitBufferNoData() {
    blink::WebDeviceProximityData& data = buffer_->data;
  }

  MockDeviceProximityListener* listener() { return listener_.get(); }
  DeviceProximityEventPumpForTesting* proximity_pump() {
    return proximity_pump_.get();
  }
  base::SharedMemoryHandle handle() { return handle_; }
  DeviceProximityHardwareBuffer* buffer() { return buffer_; }

 private:
  scoped_ptr<MockDeviceProximityListener> listener_;
  scoped_ptr<DeviceProximityEventPumpForTesting> proximity_pump_;
  base::SharedMemoryHandle handle_;
  base::SharedMemory shared_memory_;
  DeviceProximityHardwareBuffer* buffer_;

  DISALLOW_COPY_AND_ASSIGN(DeviceProximityEventPumpTest);
};

TEST_F(DeviceProximityEventPumpTest, DidStartPolling) {
  base::MessageLoop loop;

  InitBuffer();
  proximity_pump()->Start(listener());
  proximity_pump()->OnDidStart(handle());

  base::MessageLoop::current()->Run();

  const blink::WebDeviceProximityData& received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_proximity());
  EXPECT_EQ(1, static_cast<double>(received_data.value));
  EXPECT_EQ(1, static_cast<double>(received_data.min));
  EXPECT_EQ(1, static_cast<double>(received_data.max));
}

TEST_F(DeviceProximityEventPumpTest, FireAllNullEvent) {
  base::MessageLoop loop;

  InitBufferNoData();
  proximity_pump()->Start(listener());
  proximity_pump()->OnDidStart(handle());

  base::MessageLoop::current()->Run();

  const blink::WebDeviceProximityData& received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_proximity());
  EXPECT_FALSE(received_data.value);
  EXPECT_FALSE(received_data.min);
  EXPECT_FALSE(received_data.max);
}

TEST_F(DeviceProximityEventPumpTest, UpdateRespectsProximityThreshold) {
  base::MessageLoop loop;

  InitBuffer();
  proximity_pump()->Start(listener());
  proximity_pump()->OnDidStart(handle());

  base::MessageLoop::current()->Run();

  const blink::WebDeviceProximityData& received_data = listener()->data();
  EXPECT_TRUE(listener()->did_change_device_proximity());
  EXPECT_EQ(1, static_cast<double>(received_data.value));
  EXPECT_EQ(1, static_cast<double>(received_data.min));
  EXPECT_EQ(1, static_cast<double>(received_data.max));

  buffer()->data.value = 1;
  listener()->set_did_change_device_proximity(false);

  // Reset the pump's listener.
  proximity_pump()->Start(listener());

  base::MessageLoop::current()->PostTask(FROM_HERE,
      base::Bind(&DeviceProximityEventPumpForTesting::FireEvent,
                 base::Unretained(proximity_pump())));
  base::MessageLoop::current()->Run();

  EXPECT_FALSE(listener()->did_change_device_proximity());
  EXPECT_TRUE(received_data.allAvailableSensorsAreActive);
  EXPECT_EQ(1, static_cast<double>(received_data.value));
  EXPECT_EQ(1, static_cast<double>(received_data.min));
  EXPECT_EQ(1, static_cast<double>(received_data.max));
}

}  // namespace content
