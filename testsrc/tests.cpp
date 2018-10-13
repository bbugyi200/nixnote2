#include <QtTest/QtTest>
#include <QObject>
#include <QString>
#include <QHash>
#include <QPair>

#include "tests.h"
#include "../src/html/enmlformatter.h"
#include "../src/logger/qslog.h"
#include "../src/logger/qslogdest.h"

#define SET_LOGLEVEL_DEBUG QsLogging::Logger &logger = QsLogging::Logger::instance(); logger.setLoggingLevel(QsLogging::DebugLevel);

Tests::Tests(QObject *parent) :
        QObject(parent) {

}


QString Tests::formatToEnml(QString source) {
    bool guiAvailable = true;
    QHash<QString, QPair<QString, QString> > passwordSafe;
    QString cryptoJarPath;
    EnmlFormatter formatter(source, guiAvailable, passwordSafe, cryptoJarPath);
    formatter.rebuildNoteEnml();

    QString resourceStr;
    QList<qint32> resources = formatter.getResources();
    for (const auto &resource : resources) {
        if (!resourceStr.isEmpty()) {
            resourceStr.append(",");
        }
        resourceStr.append(QString::number(resource));
    }

    QString res = formatter.getContent().replace("\n", "");
    if (!resourceStr.isEmpty()) {
        res.append(" " + resourceStr);
    }
    return res;
}

QString Tests::addEnmlEnvelope(QString source, QString resources, QString bodyAttrs) {
    QString res(
            QStringLiteral(
                    R"R(<?xml version="1.0" encoding="UTF-8"?><!DOCTYPE en-note SYSTEM 'http://xml.evernote.com/pub/enml2.dtd'>)R"
            ));
    res.append("<en-note");
    if (!bodyAttrs.isEmpty()) {
        res.append(" " + bodyAttrs);
    }
    res.append(">");

    res.append(source);
    res.append(QStringLiteral("</en-note>"));

    if (!resources.isEmpty()) {
        res.append(" " + resources);
    }
    return res;
}

void Tests::enmlBasicTest() {
    QString src1("aa");
    QCOMPARE(formatToEnml(src1), addEnmlEnvelope(src1));

    QString src2("<div>aa</div>");
    QCOMPARE(formatToEnml(src2), addEnmlEnvelope(src2));

    QString src3(
            R"R(<html style="xx:1"><head style="xx:1"><title style="xx:1">xx</title></head><body style="word-wrap: break-word; -webkit-nbsp-mode: space; -webkit-line-break: after-white-space;"><div>aa</div></body></html>)R");
    QString src3r("<div>aa</div>");


    QString bodyAttr(
            R"R(style="word-wrap: break-word; -webkit-nbsp-mode: space; -webkit-line-break: after-white-space;")R"
    );
    QCOMPARE(formatToEnml(src3), addEnmlEnvelope(src3r, QString(), bodyAttr));
}

void Tests::enmlTidyTest() {
    {
        QString src("<div>aa1</xdiv>");
        QString result("<div>aa1</div>");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result));
    }
    {
        QString src("<DIV>bb1</DIV>");
        QString result("<div>bb1</div>");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result));
    }
    {
        QString src("aa2</div>");
        QString result("aa2");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result));
    }
    {
        QString src("<html>aa3</div>");
        QString result("aa3");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result));
    }
    {
        QString src("<table <tr>aa4</td>");
        QString result("aa4");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result));
    }
    // raw string literals: https://en.cppreference.com/w/cpp/language/string_literal
    {
        // defined attribute is NOT deleted
        QString src(R"R(<div style="something: 1">aa5</div>)R");
        QString result(R"R(<div style="something: 1">aa5</div>)R");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result));
    }
    {
        // undefined attributes are deleted
        QString src(
                R"R(<div style="something: 1" abcd="something: 1" lid="12" onclick="alert('hey'\)">aa6</div>)R");
        QString result(R"R(<div style="something: 1">aa6</div>)R");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result));
    }
}

void Tests::enmlNixnoteTodoTest() {
    {
        QString src("<input>");
        QString result("");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result));
    }
    {
        QString src(R"R(<input  type="checkbox"  >)R");
        QString result("<en-todo/>");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result));
    }
    {
        QString src(R"R(<input  type="checkbox"  role="button">)R");
        QString result("<en-todo/>");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result));
    }
    {
        QString src(
                R"R(<input checked="checked" type="checkbox" onclick="if(!checked) removeAttribute('checked'); else setAttribute('checked', 'checked'); editorWindow.editAlert();" style="cursor: hand;">)R");
        QString result(R"R(<en-todo checked="true"/>)R");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result));
    }
}


void Tests::enmlNixnoteImageTest() {
    {
        QString src(
                R"R(<img src="file:///home/robert7/.nixnote/db-2/dba/45875.png" type="image/png" hash="8926e14a9c5e1b6314f28ca950543f3e" oncontextmenu="window.browser.imageContextMenu('45875', '45875.png');" en-tag="en-media" style="cursor: default;" lid="45875">)R");
        QString result(
                R"R(<en-media type="image/png" hash="8926e14a9c5e1b6314f28ca950543f3e"></en-media>)R");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result, QStringLiteral("45875")));
    }
}

// note use string as params, not expressions
#define QCOMPAREX(r1, r2) if (QString::compare(r1,r2) != 0) { QLOG_WARN() << "DIFF r1: " << r1 << ", r2: " << r2; } QCOMPARE(r1, r2);

void Tests::enmlNixnoteLinkTest() {
    {
        QString src(
                R"R(<a type="application/pdf" hash="3a3fe16e6e4216802f41c40a3af59856" href="nnres:/home/robert7/.nixnote/db-2/dba/45877.pdf" oncontextmenu="window.browserWindow.resourceContextMenu('/home/robert7/.nixnote/tmp-2/45877------.pdf');" en-tag="en-media" lid="45877" title="nnres:/home/robert7/.nixnote/db-2/dba/45877.pdf">)R");
        QString result(
                R"R(<en-media type="application/pdf" hash="3a3fe16e6e4216802f41c40a3af59856"></en-media>)R");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result, QStringLiteral("45877")));
    }
    // xls+zip
    {
        QString src(
                R"R(<a en-tag="en-media" lid="45878" type="application/vnd.oasis.opendocument.spreadsheet" hash="2bb10cc981690fc6b87e50b825667bbb" href="nnres:/home/robert7/.nixnote/db-2/dba/45878.ods" oncontextmenu="window.browserWindow.resourceContextMenu(&amp;apos/home/robert7/.nixnote/db-2/dba/45878.ods&amp;apos);"><img en-tag="temporary" title="qs-qs.ods" src="file:///&lt;img en-tag=" temporary"=""></a><br><a type="application/zip" hash="eb57834ba58527ea4d4422f7fbf4498c" href="nnres:/home/robert7/.nixnote/db-2/dba/45880.zip" oncontextmenu="window.browserWindow.resourceContextMenu('/home/robert7/.nixnote/tmp-2/45880------.zip');" en-tag="en-media" lid="45880" title="nnres:/home/robert7/.nixnote/db-2/dba/45880.zip"><img src="file:////home/robert7/.nixnote/tmp-2/45880_icon.png" title="shortcuts.zip" en-tag="temporary"></a>)R");
        QString result(
                R"R(<en-media type="application/vnd.oasis.opendocument.spreadsheet" hash="2bb10cc981690fc6b87e50b825667bbb"></en-media><br /><en-media type="application/zip" hash="eb57834ba58527ea4d4422f7fbf4498c"></en-media>)R");

        const QString &r1 = formatToEnml(src);
        const QString &r2 = addEnmlEnvelope(result, QStringLiteral("45878,45880"));
        QCOMPAREX(r1, r2); //note use string, not expressions
    }
    // zip
    {
        QString src(
                R"R(<a type="application/zip" hash="eb57834ba58527ea4d4422f7fbf4498c" href="nnres:/home/robert7/.nixnote/db-2/dba/45880.zip" oncontextmenu="window.browserWindow.resourceContextMenu('/home/robert7/.nixnote/tmp-2/45880------.zip');" en-tag="en-media" lid="45880" title="nnres:/home/robert7/.nixnote/db-2/dba/45880.zip"><img src="file:////home/robert7/.nixnote/tmp-2/45880_icon.png" title="shortcuts.zip" en-tag="temporary"></a>)R");
        QString result(
                R"R(<en-media type="application/zip" hash="eb57834ba58527ea4d4422f7fbf4498c"></en-media>)R");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result, QStringLiteral("45880")));
    }
}

void Tests::enmlNixnoteObjectTest() {
    {
        QString src(
                R"R(<div><object style="width:100%; height: 600px" hash="817602cc08f9237ed641ed1703784eca" type="application/pdf" lid="45883"></object><br></div>)R");
        QString result(
                R"R(<div><en-media type="application/pdf" hash="817602cc08f9237ed641ed1703784eca"></en-media><br /></div>)R");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result, QStringLiteral("45883")));
    }
}

void Tests::enmlNixnoteEncryptTest() {
    {
        QString src(
                R"R(<div><table border="1" width="100%" class=")R"
                HTML_TEMP_TABLE_CLASS
                R"R("> <tbody> <tr> <td><br> aaaaa</td> </tr></tbody>)R"
                R"R(</table><img en-tag="en-crypt" cipher="RC2" hint="qq" length="64" alt="bGHOocsWJD4Id76YevNUb29Lxi7/aCAI" src="file:///usr/share/nixnote2/images/encrypt.png" id="crypt1" onmouseover="style.cursor='hand'" onclick="window.browserWindow.decryptText('crypt1', 'bGHOocsWJD4Id76YevNUb29Lxi7/aCAI', 'qq', 'RC2', 64);" style="display:block"></div>)R"
                );
        QString result(
                R"R(<div><en-crypt cipher="RC2" length="64" hint="qq">bGHOocsWJD4Id76YevNUb29Lxi7/aCAI</en-crypt></div>)R");
        QCOMPARE(formatToEnml(src), addEnmlEnvelope(result));
    }
}





//

QT_BEGIN_NAMESPACE
QTEST_ADD_GPU_BLACKLIST_SUPPORT_DEFS

QT_END_NAMESPACE

int main(int argc, char *argv[]) {
    QsLogging::Logger &logger = QsLogging::Logger::instance();
    logger.setLoggingLevel(QsLogging::WarnLevel);
    QsLogging::DestinationPtr debugDestination(QsLogging::DestinationFactory::MakeDebugOutputDestination());
    logger.addDestination(debugDestination.get());

    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);

    QTEST_DISABLE_KEYPAD_NAVIGATION
    QTEST_ADD_GPU_BLACKLIST_SUPPORT

    Tests tc;

    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}
