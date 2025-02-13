#include <QString>
#include <QDebug>
#include "minecraft/LaunchContext.h"
#ifdef Q_OS_MACOS
#include <sys/sysctl.h>
#endif
#include <FileSystem.h>
#include <QStandardPaths>
#include <QFileInfo>
#include "MessageLevel.h"
#include <QFile>
#include <QProcess>
#include <QMap>
#include "java/JavaUtils.h"
#include "FileSystem.h"
#include "Commandline.h"
#include "Application.h"

#ifdef Q_OS_MACOS
bool rosettaDetect() {
    int ret = 0;
    size_t size = sizeof(ret);
    if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) == -1)
    {
        return false;
    }
    if(ret == 0)
    {
        return false;
    }
    if(ret == 1)
    {
        return true;
    }
    return false;
}
#endif

namespace SysInfo {
QString currentSystem() {
#if defined(Q_OS_LINUX)
    return "linux";
#elif defined(Q_OS_MACOS)
    return "osx";
#elif defined(Q_OS_WINDOWS)
    return "windows";
#elif defined(Q_OS_FREEBSD)
    return "freebsd";
#elif defined(Q_OS_OPENBSD)
    return "openbsd";
#else
    return "unknown";
#endif
}

QString useQTForArch(){
    auto qtArch = QSysInfo::currentCpuArchitecture();
#if defined(Q_OS_MACOS) && !defined(Q_PROCESSOR_ARM)
    if(rosettaDetect())
    {
        return "arm64";
    }
    else
    {
        return "x86_64";
    }
#endif
    return qtArch;
}

QString runCheckerForArch(LaunchContext launchContext){
    QString checkerJar = JavaUtils::getJavaCheckPath();

    if (checkerJar.isEmpty())
    {
        qDebug() << "Java checker library could not be found. Please check your installation.";
        return useQTForArch();
    }

    QStringList args;

    QProcessPtr process = new QProcess();
    args.append({"-jar", checkerJar});
    process->setArguments(args);
    process->setProgram(launchContext.getJavaPath().toString());
    process->setProcessChannelMode(QProcess::SeparateChannels);
    process->setProcessEnvironment(CleanEnviroment());
    qDebug() << "Running java checker: " + launchContext.getJavaPath().toString() + args.join(" ");;

    process->start();
    if(!process->waitForFinished(15000)){
        // we've been waiting for 15 seconds! wtf! OR... it already finished. But HOW WOULD THAT HAPPEN?
        process->kill(); // die. BUUURNNNN
        // fallback to using polymc arch
        return useQTForArch();
    } else {
        // yay we can use the java arch
        QString stdout_javaChecker;
        QString stderr_javaChecker;

        // process stdout
        QByteArray data = process->readAllStandardOutput();
        QString added = QString::fromLocal8Bit(data);
        added.remove('\r');
        stdout_javaChecker += added;

        // process stderr
        data = process->readAllStandardError();
        added = QString::fromLocal8Bit(data);
        added.remove('\r');
        stderr_javaChecker += added;

        QMap<QString, QString> results;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        QStringList lines = stdout_javaChecker.split("\n", Qt::SkipEmptyParts);
#else
        QStringList lines = stdout_javaChecker.split("\n", QString::SkipEmptyParts);
#endif
        for(QString line : lines)
        {
            line = line.trimmed();
            // NOTE: workaround for GH-4125, where garbage is getting printed into stdout on bedrock linux
            if (line.contains("/bedrock/strata")) {
                continue;
            }

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            auto parts = line.split('=', Qt::SkipEmptyParts);
#else
            auto parts = line.split('=', QString::SkipEmptyParts);
#endif
            if(parts.size() != 2 || parts[0].isEmpty() || parts[1].isEmpty())
            {
                continue;
            }
            else
            {
                results.insert(parts[0], parts[1]);
            }
        }

        if(!results.contains("os.arch") || !results.contains("java.version") || !results.contains("java.vendor"))
        {
            // wtf man why
            // fallback to using polymc arch
            return useQTForArch();
        }

        return results["os.arch"];
    }
}

QString currentArch(LaunchContext launchContext) {
    auto realJavaArchitecture = launchContext.getRealJavaArch().toString();
    if(realJavaArchitecture == ""){
        //BRO WHY NOW I HAVE TO USE A JAVA CHECKER >:(
        qDebug() << "SysInfo: BRO I HAVE TO USE A JAVA CHECKER WHY IS JAVA ARCH BORKED";
        launchContext.setRealJavaArch(runCheckerForArch(launchContext));
        realJavaArchitecture = launchContext.getRealJavaArch().toString();
    }
    //qDebug() << "SysInfo: realJavaArch = " << realJavaArchitecture;
    if(realJavaArchitecture == "aarch64"){
        return "arm64";
    } else {
        return realJavaArchitecture;
    }
}
}

