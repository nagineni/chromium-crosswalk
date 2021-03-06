// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/input/input_param_traits.h"

#include "content/common/input/input_event.h"
#include "content/common/input_messages.h"
#include "ipc/ipc_message.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"

namespace content {
namespace {

typedef ScopedVector<InputEvent> InputEvents;

void AddTo(InputEvents& events, const WebKit::WebInputEvent& event) {
  events.push_back(new InputEvent(event, ui::LatencyInfo(), false));
}

class InputParamTraitsTest : public testing::Test {
 protected:
  static void Compare(const InputEvent* a, const InputEvent* b) {
    EXPECT_EQ(!!a->web_event, !!b->web_event);
    if (a->web_event && b->web_event) {
      const size_t a_size = a->web_event->size;
      ASSERT_EQ(a_size, b->web_event->size);
      EXPECT_EQ(0, memcmp(a->web_event.get(), b->web_event.get(), a_size));
    }
    EXPECT_EQ(a->latency_info.latency_components.size(),
              b->latency_info.latency_components.size());
    EXPECT_EQ(a->is_keyboard_shortcut, b->is_keyboard_shortcut);
  }

  static void Compare(const InputEvents* a, const InputEvents* b) {
    for (size_t i = 0; i < a->size(); ++i)
      Compare((*a)[i], (*b)[i]);
  }

  static void Compare(const SyntheticSmoothScrollGestureParams* a,
                      const SyntheticSmoothScrollGestureParams* b) {
    EXPECT_EQ(a->gesture_source_type, b->gesture_source_type);
    EXPECT_EQ(a->distance, b->distance);
    EXPECT_EQ(a->anchor_x, b->anchor_x);
    EXPECT_EQ(a->anchor_y, b->anchor_y);
  }

  static void Compare(const SyntheticGesturePacket* a,
                      const SyntheticGesturePacket* b) {
    ASSERT_EQ(!!a, !!b);
    if (!a) return;
    ASSERT_EQ(!!a->gesture_params(), !!b->gesture_params());
    if (!a->gesture_params()) return;
    ASSERT_EQ(a->gesture_params()->GetGestureType(),
              b->gesture_params()->GetGestureType());
    switch (a->gesture_params()->GetGestureType()) {
      case SyntheticGestureParams::SMOOTH_SCROLL_GESTURE:
        Compare(SyntheticSmoothScrollGestureParams::Cast(a->gesture_params()),
                SyntheticSmoothScrollGestureParams::Cast(b->gesture_params()));
        break;
    }
  }

  static void Verify(const InputEvents& events_in) {
    IPC::Message msg;
    IPC::ParamTraits<InputEvents>::Write(&msg, events_in);

    InputEvents events_out;
    PickleIterator iter(msg);
    EXPECT_TRUE(IPC::ParamTraits<InputEvents>::Read(&msg, &iter, &events_out));

    Compare(&events_in, &events_out);

    // Perform a sanity check that logging doesn't explode.
    std::string events_in_string;
    IPC::ParamTraits<InputEvents>::Log(events_in, &events_in_string);
    std::string events_out_string;
    IPC::ParamTraits<InputEvents>::Log(events_out, &events_out_string);
    ASSERT_FALSE(events_in_string.empty());
    EXPECT_EQ(events_in_string, events_out_string);
  }

  static void Verify(const SyntheticGesturePacket& packet_in) {
    IPC::Message msg;
    IPC::ParamTraits<SyntheticGesturePacket>::Write(&msg, packet_in);

    SyntheticGesturePacket packet_out;
    PickleIterator iter(msg);
    EXPECT_TRUE(IPC::ParamTraits<SyntheticGesturePacket>::Read(&msg, &iter,
                                                               &packet_out));

    Compare(&packet_in, &packet_out);

    // Perform a sanity check that logging doesn't explode.
    std::string packet_in_string;
    IPC::ParamTraits<SyntheticGesturePacket>::Log(packet_in, &packet_in_string);
    std::string packet_out_string;
    IPC::ParamTraits<SyntheticGesturePacket>::Log(packet_out,
                                                  &packet_out_string);
    ASSERT_FALSE(packet_in_string.empty());
    EXPECT_EQ(packet_in_string, packet_out_string);
  }
};

TEST_F(InputParamTraitsTest, UninitializedEvents) {
  InputEvent event;

  IPC::Message msg;
  IPC::WriteParam(&msg, event);

  InputEvent event_out;
  PickleIterator iter(msg);
  EXPECT_FALSE(IPC::ReadParam(&msg, &iter, &event_out));
}

TEST_F(InputParamTraitsTest, InitializedEvents) {
  InputEvents events;

  ui::LatencyInfo latency;

  WebKit::WebKeyboardEvent key_event;
  key_event.type = WebKit::WebInputEvent::RawKeyDown;
  key_event.nativeKeyCode = 5;
  events.push_back(new InputEvent(key_event, latency, false));

  WebKit::WebMouseWheelEvent wheel_event;
  wheel_event.type = WebKit::WebInputEvent::MouseWheel;
  wheel_event.deltaX = 10;
  latency.AddLatencyNumber(ui::INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT, 1, 1);
  events.push_back(new InputEvent(wheel_event, latency, false));

  WebKit::WebMouseEvent mouse_event;
  mouse_event.type = WebKit::WebInputEvent::MouseDown;
  mouse_event.x = 10;
  latency.AddLatencyNumber(ui::INPUT_EVENT_LATENCY_UI_COMPONENT, 2, 2);
  events.push_back(new InputEvent(mouse_event, latency, false));

  WebKit::WebGestureEvent gesture_event;
  gesture_event.type = WebKit::WebInputEvent::GestureScrollBegin;
  gesture_event.x = -1;
  events.push_back(new InputEvent(gesture_event, latency, false));

  WebKit::WebTouchEvent touch_event;
  touch_event.type = WebKit::WebInputEvent::TouchStart;
  touch_event.touchesLength = 1;
  touch_event.touches[0].radiusX = 1;
  events.push_back(new InputEvent(touch_event, latency, false));

  Verify(events);
}

TEST_F(InputParamTraitsTest, InvalidSyntheticGestureParams) {
  IPC::Message msg;
  // Write invalid value for SyntheticGestureParams::GestureType.
  WriteParam(&msg, -3);

  SyntheticGesturePacket packet_out;
  PickleIterator iter(msg);
  ASSERT_FALSE(
      IPC::ParamTraits<SyntheticGesturePacket>::Read(&msg, &iter, &packet_out));
}

TEST_F(InputParamTraitsTest, SyntheticSmoothScrollGestureParams) {
  scoped_ptr<SyntheticSmoothScrollGestureParams> gesture_params(
      new SyntheticSmoothScrollGestureParams);
  gesture_params->gesture_source_type = SyntheticGestureParams::TOUCH_INPUT;
  gesture_params->distance = 123;
  gesture_params->anchor_x = 234;
  gesture_params->anchor_y = 345;
  ASSERT_EQ(SyntheticGestureParams::SMOOTH_SCROLL_GESTURE,
            gesture_params->GetGestureType());
  SyntheticGesturePacket packet_in;
  packet_in.set_gesture_params(gesture_params.PassAs<SyntheticGestureParams>());

  Verify(packet_in);
}

}  // namespace
}  // namespace content
