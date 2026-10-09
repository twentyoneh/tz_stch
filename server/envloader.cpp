#include "envloader.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

bool EnvLoader::load(
    const QString &filePath,
    QMap<QString, QString> &values,
    QString &error
    )
{
    error.clear();

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QString("Cannot open .env file: %1")
        .arg(file.errorString());

        return false;
    }

    QTextStream stream(&file);
    QMap<QString, QString> loadedValues;

    const QRegularExpression keyPattern(
        "^[A-Za-z_][A-Za-z0-9_]*$"
        );

    int lineNumber = 0;

    while (!stream.atEnd()) {
        ++lineNumber;

        const QString line = stream.readLine().trimmed();

        //пропускаем пустые строки и комментарии
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        const auto separator = line.indexOf('=');

        if (separator <= 0) {
            error = QString("Expected KEY=VALUE at line %1")
            .arg(lineNumber);

            return false;
        }

        //делим строчку напополам
        const QString key = line.left(separator).trimmed();
        QString value = line.mid(separator + 1).trimmed();

        if (!keyPattern.match(key).hasMatch()) {
            error = QString("Invalid variable name at line %1")
            .arg(lineNumber);

            return false;
        }

        //убираем парные одинарные или двойные кавычки
        if (value.startsWith('"') || value.startsWith('\'')) {
            const QChar quote = value.front();

            if (value.size() < 2 || !value.endsWith(quote)) {
                error = QString("Unclosed quote at line %1")
                .arg(lineNumber);

                return false;
            }

            value = value.mid(1, value.size() - 2);
        }

        loadedValues.insert(key, value);
    }

    if (stream.status() != QTextStream::Ok) {
        error = "Failed to read .env file";
        return false;
    }

    values = loadedValues;
    return true;
}
