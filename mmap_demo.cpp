#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

#include <thread>
#include <future>
#include <algorithm>

namespace {

constexpr const auto BUF_SIZE = 1024 * 1024 * 1024; // 1GB

void runWithMmap(QFile& file)
{
    if (!file.resize(BUF_SIZE))
        qFatal("failed to resize file %s: %s", qPrintable(file.fileName()), qPrintable(file.errorString()));

    auto buffer = file.map(0, BUF_SIZE);
    if (!buffer)
        qFatal("failed to map file %s: %s", qPrintable(file.fileName()), qPrintable(file.errorString()));

    std::fill_n(buffer, BUF_SIZE, 1);

    if (!file.unmap(buffer))
        qFatal("failed to unmap file %s: %s", qPrintable(file.fileName()), qPrintable(file.errorString()));
}

void runWithoutMmap(QFile& file)
{
    std::vector<uchar> buf(BUF_SIZE);
    std::fill(buf.begin(), buf.end(), 1);

    file.write(reinterpret_cast<const char*>(buf.data()), buf.size());
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const auto useMmap = app.arguments().constLast() == QLatin1String("mmap");
    qInfo("using mmap: %d", useMmap);
    QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    if (!cacheDir.exists() && !cacheDir.mkpath(cacheDir.absolutePath()))
        qFatal("failed to create cache dir: %s", qPrintable(cacheDir.absolutePath()));

    std::vector<std::future<void>> tasks(std::thread::hardware_concurrency());
    std::generate(tasks.begin(), tasks.end(), [useMmap, index=1, cacheDir]() mutable {
        const auto filePath = cacheDir.absoluteFilePath(QString::number(index++));
        return std::async(std::launch::async, [useMmap, filePath]() {
            QFile file(filePath);
            if (file.exists() && !file.remove())
                qFatal("failed to remove file %s: %s", qPrintable(filePath), qPrintable(file.errorString()));

            if (!file.open(QIODevice::ReadWrite))
                qFatal("failed to open file %s: %s", qPrintable(filePath), qPrintable(file.errorString()));

            useMmap ? runWithMmap(file) : runWithoutMmap(file);

            qInfo("wrote data to %s", qPrintable(filePath));
        });
    });

    return 0;
}
