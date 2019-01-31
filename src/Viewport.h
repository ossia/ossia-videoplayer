#pragma once
//! Based on code from https://github.com/mpv-player/mpv-examples
#include <QMetaMethod>
#include <QOpenGLWindow>
#include <QTimer>
#include <QtWidgets/QOpenGLWidget>

#include <mpv/client.h>
#include <mpv/opengl_cb.h>
#include <mpv/qthelper.hpp>
#include <wobjectdefs.h>

class Viewport final : public QOpenGLWindow
{
  W_OBJECT(Viewport)

public:
  Viewport();
  ~Viewport() override;

  void setFile(const QString& s)
  {
    command(QStringList {"loadfile", s});
  }
  void seek(double pos)
  {
    QMetaObject::invokeMethod(
        this, [&] { command(QVariantList() << "seek" << pos << "absolute"); },
        Qt::QueuedConnection);
  }
  void play(bool b)
  {
    setOption("pause", !b);
  }
  void loop(bool b)
  {
    setOption("loop", b ? "inf" : "0");
  }
  void setSpeed(double s)
  {
    setOption("speed", s);
  }

  void durationChanged(double value)
  W_SIGNAL(durationChanged, value);
  void positionChanged(double value)
  W_SIGNAL(positionChanged, value);

private:
  void command(const QVariant& params);
  void setOption(const QString& name, const QVariant& value);
  QVariant getOption(const QString& name) const;

  void initializeGL() override;
  void paintGL() override;

  void maybeUpdate();

  void on_mpv_events();
  void handle_mpv_event(mpv_event* event);

  mpv::qt::Handle mpv;
  mpv_opengl_cb_context* m_glCtx{};
};
