// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_WEBKIT_TEST_RUNNER_H_
#define CONTENT_SHELL_WEBKIT_TEST_RUNNER_H_

#include "base/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/renderer/render_view_observer.h"
#include "third_party/WebKit/Tools/DumpRenderTree/chromium/TestRunner/public/WebPreferences.h"
#include "third_party/WebKit/Tools/DumpRenderTree/chromium/TestRunner/public/WebTestDelegate.h"
#include "v8/include/v8.h"

class SkCanvas;

namespace WebKit {
struct WebRect;
}

namespace WebTestRunner {
class WebTestProxyBase;
}

namespace content {

// This is the renderer side of the webkit test runner.
class WebKitTestRunner : public RenderViewObserver,
                         public WebTestRunner::WebTestDelegate {
 public:
  explicit WebKitTestRunner(RenderView* render_view);
  virtual ~WebKitTestRunner();

  // RenderViewObserver implementation.
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;
  virtual void DidClearWindowObject(WebKit::WebFrame* frame) OVERRIDE;
  virtual void DidFinishLoad(WebKit::WebFrame* frame) OVERRIDE;
  virtual void DidRequestShowContextMenu(
      WebKit::WebFrame* frame,
      const WebKit::WebContextMenuData& data) OVERRIDE;

  // WebTestDelegate implementation.
  virtual void clearContextMenuData();
  virtual void clearEditCommand();
  virtual void fillSpellingSuggestionList(
      const WebKit::WebString& word,
      WebKit::WebVector<WebKit::WebString>* suggestions);
  virtual void setEditCommand(const std::string& name,
                              const std::string& value);
  virtual WebKit::WebContextMenuData* lastContextMenuData() const;
  virtual void setGamepadData(const WebKit::WebGamepads& gamepads);
  virtual void printMessage(const std::string& message);
  virtual void postTask(::WebTestRunner::WebTask* task);
  virtual void postDelayedTask(::WebTestRunner::WebTask* task,
                               long long ms);
  virtual WebKit::WebString registerIsolatedFileSystem(
      const WebKit::WebVector<WebKit::WebString>& absolute_filenames);
  virtual long long getCurrentTimeInMillisecond();
  virtual WebKit::WebString getAbsoluteWebStringFromUTF8Path(
      const std::string& utf8_path);
  virtual WebKit::WebURL localFileToDataURL(const WebKit::WebURL& file_url);
  virtual WebKit::WebURL rewriteLayoutTestsURL(const std::string& utf8_url);
  virtual ::WebTestRunner::WebPreferences* preferences();
  virtual void applyPreferences();
  virtual std::string makeURLErrorDescription(const WebKit::WebURLError& error);

  void Reset();
  void NotifyDone();
  void DumpAsText();
  void DumpChildFramesAsText();
  void WaitUntilDone();
  void OverridePreference(const std::string& key, v8::Local<v8::Value> value);

  void NotImplemented(const std::string& object, const std::string& method);

  void set_proxy(::WebTestRunner::WebTestProxyBase* proxy) { proxy_ = proxy; }

 private:
  // Message handlers.
  void OnSetTestConfiguration(
      const base::FilePath& current_working_directory,
      bool enable_pixel_dumping,
      int layout_test_timeout,
      bool allow_external_pages,
      const std::string& expected_pixel_hash);

  void CaptureDump();
  void CaptureTextDump();
  void CaptureImageDump();
  SkCanvas* GetCanvas();
  void PaintRect(const WebKit::WebRect& rect);
  void PaintInvalidatedRegion();
  void DisplayRepaintMask();

  scoped_ptr<SkCanvas> canvas_;
  scoped_ptr<WebKit::WebContextMenuData> last_context_menu_data_;
  base::FilePath current_working_directory_;

  ::WebTestRunner::WebTestProxyBase* proxy_;

  ::WebTestRunner::WebPreferences prefs_;

  bool test_is_running_;
  bool wait_until_done_;
  bool load_finished_;
  bool dump_as_text_;
  bool dump_child_frames_as_text_;
  bool printing_;

  bool enable_pixel_dumping_;
  int layout_test_timeout_;
  bool allow_external_pages_;
  std::string expected_pixel_hash_;

  DISALLOW_COPY_AND_ASSIGN(WebKitTestRunner);
};

}  // namespace content

#endif  // CONTENT_SHELL_WEBKIT_TEST_RUNNER_H_
