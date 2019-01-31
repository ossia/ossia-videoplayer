#include "Viewport.h"

#include <ossia/network/common/complex_type.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/oscquery/oscquery_server.hpp>

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QTimer>

struct VideoPlayer
{
  VideoPlayer(ossia::net::node_base& root)
  {
    QTimer::singleShot(1000, [&] {
      viewport.setFile("/home/jcelerier/sample-1080p30-Hap.avi");
      viewport.setSpeed(5);
      viewport.loop(true);
      viewport.play(true);
    });
    auto& node = *root.create_child("video.0");

    auto play = ossia::create_parameter(node, "play", "bool");
    play->add_callback([&](const ossia::value& v) {
      if (auto b = v.target<bool>())
      {
        viewport.play(*b);
      }
    });

    auto setFile = ossia::create_parameter(node, "file", "path");
    setFile->add_callback([&](const ossia::value& v) {
      if (auto path = v.target<std::string>())
      {
        viewport.setFile(QString::fromStdString(*path));
      }
    });

    auto seek = ossia::create_parameter(node, "seek", "float");
    seek->add_callback([&](const ossia::value& v) {
      if (auto val = v.target<float>())
      {
        viewport.seek(*val);
      }
    });

    auto loop = ossia::create_parameter(node, "loop", "bool");
    loop->add_callback([&](const ossia::value& v) {
      if (auto val = v.target<bool>())
      {
        viewport.loop(*val);
      }
    });

    auto speed = ossia::create_parameter(node, "speed", "float");
    speed->add_callback([&](const ossia::value& v) {
      if (auto val = v.target<float>())
      {
        viewport.setSpeed(*val);
      }
    });

    viewport.show();
  }

  Viewport viewport;
};

int main(int argc, char* argv[])
{
  QGuiApplication app {argc, argv};
  QLocale::setDefault(QLocale::C);
  setlocale(LC_ALL, "C");

  QCoreApplication::setApplicationName("ossia-videoplayer");
  QCoreApplication::setApplicationVersion("1.0");

  QCommandLineParser parser;
  parser.setApplicationDescription(
      "Expose a number of video playback devices through OSCQuery");
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption targetDirectoryOption(
      QStringList() << "n"
                    << "num-videos",
      QCoreApplication::translate(
          "main", "Open as many video players as there are arguments."),
      QCoreApplication::translate("main", "N"));
  parser.addOption(targetDirectoryOption);
  parser.process(app);

  const std::size_t num_videos
      = std::max(1, parser.value(targetDirectoryOption).toInt());

  ossia::oscquery_device device {1340, 5567, "video"};
  std::vector<std::unique_ptr<VideoPlayer>> players(num_videos);

  for (int i = 0; i < num_videos; i++)
    players[i] = std::make_unique<VideoPlayer>(device.device.get_root_node());

  return app.exec();
}
