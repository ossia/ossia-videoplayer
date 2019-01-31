#include "Viewport.h"

#include <QtGui/QOpenGLContext>

#include <wobjectimpl.h>

#include <stdexcept>

W_OBJECT_IMPL(Viewport)
Viewport::Viewport()
{
  mpv = mpv::qt::Handle::FromRawHandle(mpv_create());
  if (!mpv)
    throw std::runtime_error("could not create mpv context");

  mpv_set_option_string(mpv, "terminal", "yes");
  if (mpv_initialize(mpv) < 0)
    throw std::runtime_error("could not initialize mpv context");

  mpv::qt::set_option_variant(mpv, "vo", "opengl-cb");
  mpv::qt::set_option_variant(mpv, "hwdec", "auto");

  m_glCtx = (mpv_opengl_cb_context*)mpv_get_sub_api(mpv, MPV_SUB_API_OPENGL_CB);
  if (!m_glCtx)
    throw std::runtime_error("OpenGL not compiled in");

  mpv_opengl_cb_set_update_callback(
      m_glCtx,
      [](void* ctx) {
        QMetaObject::invokeMethod(
            (Viewport*)ctx, &Viewport::maybeUpdate, Qt::AutoConnection);
      },
      this);

  connect(this, &QOpenGLWindow::frameSwapped, this, [=] {
    mpv_opengl_cb_report_flip(m_glCtx, 0);
  });

  mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
  mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
  mpv_set_wakeup_callback(
      mpv,
      [](void* ctx) {
        QMetaObject::invokeMethod(
            static_cast<Viewport*>(ctx), &Viewport::on_mpv_events,
            Qt::QueuedConnection);
      },
      this);
}

Viewport::~Viewport()
{
  makeCurrent();
  mpv_opengl_cb_set_update_callback(m_glCtx, nullptr, nullptr);
  mpv_opengl_cb_uninit_gl(m_glCtx);
}

void Viewport::command(const QVariant& params)
{
  mpv::qt::command_variant(mpv, params);
}

void Viewport::setOption(const QString& name, const QVariant& value)
{
  mpv::qt::set_property(mpv, name, value);
}

QVariant Viewport::getOption(const QString& name) const
{
  return mpv::qt::get_property(mpv, name);
}

void Viewport::initializeGL()
{
  auto get_proc_address = [](void* ctx, const char* name) -> void* {
    Q_UNUSED(ctx);
    QOpenGLContext* glctx = QOpenGLContext::currentContext();
    if (!glctx)
      return nullptr;
    return reinterpret_cast<void*>(glctx->getProcAddress(QByteArray(name)));
  };
  int r = mpv_opengl_cb_init_gl(m_glCtx, nullptr, get_proc_address, nullptr);
  if (r < 0)
    throw std::runtime_error("could not initialize OpenGL");
}

void Viewport::paintGL()
{
  mpv_opengl_cb_draw(m_glCtx, defaultFramebufferObject(), width(), -height());
}

void Viewport::on_mpv_events()
{
  // Process all events, until the event queue is empty.
  while (mpv)
  {
    mpv_event* event = mpv_wait_event(mpv, 0);
    if (event->event_id == MPV_EVENT_NONE)
    {
      break;
    }
    handle_mpv_event(event);
  }
}

void Viewport::handle_mpv_event(mpv_event* event)
{
  switch (event->event_id)
  {
    case MPV_EVENT_PROPERTY_CHANGE:
    {
      mpv_event_property* prop = (mpv_event_property*)event->data;
      QLatin1String prop_name {prop->name};
      if (prop_name == "time-pos")
      {
        if (prop->format == MPV_FORMAT_DOUBLE)
        {
          double time = *(double*)prop->data;
          positionChanged(time);
        }
      }
      else if (prop->name == "duration")
      {
        if (prop->format == MPV_FORMAT_DOUBLE)
        {
          double time = *(double*)prop->data;
          durationChanged(time);
        }
      }
      break;
    }
    default:;
      // Ignore uninteresting or unknown events.
  }
}

// Make Qt invoke mpv_opengl_cb_draw() to draw a new/updated video frame.
void Viewport::maybeUpdate()
{
  // If the Qt window is not visible, Qt's update() will just skip rendering.
  // This confuses mpv's opengl-cb API, and may lead to small occasional
  // freezes due to video rendering timing out.
  // Handle this by manually redrawing.
  // Note: Qt doesn't seem to provide a way to query whether update() will
  //       be skipped, and the following code still fails when e.g. switching
  //       to a different workspace with a reparenting window manager.

  if (windowStates() & Qt::WindowMinimized)
  {
    makeCurrent();
    paintGL();
    context()->swapBuffers(context()->surface());
    mpv_opengl_cb_report_flip(m_glCtx, 0);
    doneCurrent();
  }
  else
  {
    update();
  }
}
