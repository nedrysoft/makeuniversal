/*
 * (C) 2020 Adrian Carpenter (https://github.com/fizzyade)
 *
 * This product is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This product is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this product.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <QCoreApplication>
#include <QDirIterator>
#include <QProcess>
#include <QDebug>
#include <QRegularExpression>

enum Arch {
    x86_64,
    arm64
};

enum FileType {
    Error,
    HasArch,
    NoArch,
    NotBinary
};

FileType checkArch(QString filename, Arch arch)
{
    QProcess lipo;
    QString archString;

    if (arch==x86_64) {
        archString = "x86_64";
    } else {
        archString = "arm64";
    }

    lipo.start("lipo", QStringList() << filename << "-verify_arch" << archString);

    if (!lipo.waitForStarted())
       return Error;

    if (!lipo.waitForFinished(-1))
       return Error;

    QByteArray result = lipo.readAllStandardError();

    if (lipo.exitCode()==1) {
        QRegularExpression re("can\\'t figure out the architecture type of");
        QRegularExpressionMatch match = re.match(QString::fromLatin1(result));

        if (match.hasMatch()) {
            return NotBinary;
        }

        return(NoArch);
    }

    return(HasArch);
}

bool copyFolder(QString source, QString destination)
{
    QProcess rsync;

    rsync.start("rsync", QStringList() << "-av" << "--progress" << "-l" << "-r" << source << destination);

    if (!rsync.waitForStarted())
       return 1;

    if (!rsync.waitForFinished(-1))
       return 1;

    QByteArray result = rsync.readAllStandardError();

    qDebug() << rsync.exitCode() << result;

    if (rsync.exitCode()==0) {
        return 0;
    }

    return 1;
}

bool lipo(QString destination, QString source)
{
    QProcess lipo;

    lipo.start("lipo", QStringList() << "-create" << "-output" << destination << destination << source);

    if (!lipo.waitForStarted())
       return 1;

    if (!lipo.waitForFinished(-1))
       return 1;

    QByteArray result = lipo.readAllStandardError();

    qDebug() << lipo.exitCode() << result;

    return lipo.exitCode();
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    int failures = 0;
    int success = 0;
    int skipped = 0;

    if (argc!=4) {
        qDebug() << "usage: makeuniversal <destination> <qtbase x86_64 folder> <qtbase arn64 folder>";

        return -1;
    }

    QString x86_64Root = QFileInfo(argv[2]).absoluteFilePath()+"/";
    QString arm64Root = QFileInfo(argv[3]).absoluteFilePath()+"/";
    QString universalRoot = QFileInfo(argv[1]).absoluteFilePath();

    qDebug() << "copying qt distribution from x86_64 to destination (this may take a while)...";

    copyFolder(x86_64Root, universalRoot);

    qDebug() << "creating universal binaries...";

    QDirIterator it(universalRoot, QDir::NoFilter, QDirIterator::Subdirectories);

    while(it.hasNext()) {
        it.next();

        if ((it.fileName()==".") || (it.fileName()==".."))
            continue;

        QFileInfo fileInfo(it.filePath());

        if (fileInfo.isDir() || fileInfo.isSymLink())
            continue;

        QString x86_64File = it.filePath();
        QString rootPath = it.path();
        QString subPath = x86_64File.mid(rootPath.length());

        FileType archType = checkArch(it.filePath(), arm64);

        if (archType==NoArch) {
            QString arm64File = QFileInfo(arm64Root).absolutePath()+subPath;

            if (checkArch(arm64File, arm64)==HasArch) {
                if (lipo(x86_64File, arm64File)==0) {
                    qDebug() << "success adding arm64 arch to binary:" << subPath;

                    success++;
                } else {
                    qDebug() << "failed adding arm64 arch to binary:" << subPath;

                    failures++;
                }
            }
        } else {
            if (archType==HasArch) {
                qDebug() << "skipped adding arm64 arch to binary:" << subPath;

                skipped++;
            }
        }
    }

    if (success+failures+skipped) {
        qDebug();
    }

    qDebug() << "Total binaries:" << (success+failures+skipped) << ", Skipped:" << skipped << ", Failed:" << failures;

    return 0;
}
