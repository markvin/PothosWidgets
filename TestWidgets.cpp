// Copyright (c) 2026 Pothos contributors
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>
#include <Pothos/Object/Containers.hpp>
#include <QApplication>
#include <QCoreApplication>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/***********************************************************************
 * Smoke tests of the widgets: make each block, call its setters, and
 * read the values back. The tests need no display: when no application
 * is present, the test makes one with the offscreen platform.
 **********************************************************************/

//use the application of the host (for example PothosFlow) when it has one,
//else make one QApplication that stays until the module unloads. Do not
//leak it: the core unloads the module and Qt Widgets before the process
//ends, and a QApplication that stays after that crashes at exit.
static std::unique_ptr<QApplication> testApplication;

static void ensureApplication(void)
{
    if (QApplication::instance() != nullptr) return;
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", "offscreen");
    static int argc = 1;
    static char arg0[] = "PothosWidgetsTest";
    static char *argv[] = {arg0, nullptr};
    testApplication = std::make_unique<QApplication>(argc, argv);
}

//run the queued calls of the setters; a queued slot can queue a
//new call (for example DropDown::__setOptions), thus do more rounds
static void processEvents(void)
{
    for (int i = 0; i < 10; i++)
    {
        QCoreApplication::sendPostedEvents();
        QCoreApplication::processEvents();
    }
}

//record the Qt warnings while a test runs: a queued call to a missing slot,
//or with an argument type that Qt does not know, gives only a warning
class QtWarningCheck
{
public:
    QtWarningCheck(void)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _warnings.clear();
        _previous = qInstallMessageHandler(&QtWarningCheck::handler);
    }

    ~QtWarningCheck(void)
    {
        qInstallMessageHandler(_previous);
    }

    //call after processEvents(): fail when Qt gave a warning
    void check(void)
    {
        std::vector<std::string> warnings;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            warnings.swap(_warnings);
        }
        for (const auto &warning : warnings) std::cerr << "Qt warning: " << warning << std::endl;
        POTHOS_TEST_TRUE(warnings.empty());
    }

private:
    static void handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        if (type == QtWarningMsg or type == QtCriticalMsg)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _warnings.push_back(msg.toStdString());
        }
        if (_previous != nullptr) _previous(type, context, msg);
    }

    static inline std::mutex _mutex;
    static inline std::vector<std::string> _warnings;
    static inline QtMessageHandler _previous = nullptr;
};

static Pothos::Proxy makeWidget(const std::string &path)
{
    ensureApplication();
    auto block = Pothos::BlockRegistry::make(path);
    POTHOS_TEST_TRUE(block.call<QWidget *>("widget") != nullptr);
    return block;
}

static Pothos::ObjectVector makeOptions(void)
{
    Pothos::ObjectVector options;
    for (const std::string name : {"a", "b", "c"})
    {
        Pothos::ObjectVector option;
        option.emplace_back(Pothos::Object(name + " label"));
        option.emplace_back(Pothos::Object(name));
        options.emplace_back(Pothos::Object(option));
    }
    return options;
}

POTHOS_TEST_BLOCK("/widgets/tests", test_slider)
{
    QtWarningCheck qtWarnings;
    ensureApplication();
    for (const std::string orientation : {"Horizontal", "Vertical"})
    {
        auto block = Pothos::BlockRegistry::make("/widgets/slider", orientation);
        POTHOS_TEST_TRUE(block.call<QWidget *>("widget") != nullptr);
        block.call("setTitle", QString("Slider"));
        block.call("setMinimum", -10.0);
        block.call("setMaximum", 10.0);
        block.call("setValue", 5.0);
        processEvents();
        qtWarnings.check();
        POTHOS_TEST_CLOSE(block.call<double>("value"), 5.0, 1e-9);
    }
}

POTHOS_TEST_BLOCK("/widgets/tests", test_numeric_entry)
{
    QtWarningCheck qtWarnings;
    auto block = makeWidget("/widgets/numeric_entry");
    block.call("setTitle", QString("Entry"));
    block.call("setMinimum", -10.0);
    block.call("setMaximum", 10.0);
    block.call("setDecimals", 2);
    block.call("setValue", 7.25);
    processEvents();
    qtWarnings.check();
    POTHOS_TEST_CLOSE(block.call<double>("value"), 7.25, 1e-9);
}

POTHOS_TEST_BLOCK("/widgets/tests", test_odometer)
{
    QtWarningCheck qtWarnings;
    auto block = makeWidget("/widgets/odometer");
    block.call("setTitle", QString("Odometer"));
    block.call("setValueMin", qint64(0));
    block.call("setValueMax", qint64(99999));
    block.call("setValue", qint64(1234));
    processEvents();
    qtWarnings.check();
    POTHOS_TEST_EQUAL(block.call<qint64>("value"), 1234);

    //make the digits of the indicator two times (the second time deletes
    //the first digits), then read the digits back in their order
    block.call("setSize", 6);
    processEvents();
    block.call("setSize", 4);
    block.call("setFontSize", 12);
    processEvents();
    block.call("setValue", qint64(4321));
    processEvents();
    qtWarnings.check();
    POTHOS_TEST_EQUAL(block.call<qint64>("value"), 4321);
    QStringList digits;
    for (const auto label : block.call<QWidget *>("widget")->findChildren<QLabel *>()) digits.append(label->text());
    std::cout << "digits: " << digits.join(",").toStdString() << std::endl;
    POTHOS_TEST_TRUE(digits == QStringList({"4", "3", "2", "1"}));
}

POTHOS_TEST_BLOCK("/widgets/tests", test_planar_select)
{
    QtWarningCheck qtWarnings;
    auto block = makeWidget("/widgets/planar_select");
    block.call("setTitle", QString("Planar"));
    block.call("setMinimum", std::vector<double>{-1.0, -1.0});
    block.call("setMaximum", std::vector<double>{1.0, 1.0});
    block.call("setValue", std::vector<double>{0.25, -0.5});
    processEvents();
    qtWarnings.check();
    POTHOS_TEST_EQUALV(block.call<std::vector<double>>("value"), (std::vector<double>{0.25, -0.5}));
}

POTHOS_TEST_BLOCK("/widgets/tests", test_drop_down)
{
    QtWarningCheck qtWarnings;
    auto block = makeWidget("/widgets/drop_down");
    block.call("setTitle", QString("DropDown"));
    block.call("setOptions", makeOptions());
    block.call("setValue", Pothos::Object(std::string("b")));
    processEvents();
    qtWarnings.check();
    POTHOS_TEST_EQUAL(block.call<Pothos::Object>("value").convert<std::string>(), "b");
}

POTHOS_TEST_BLOCK("/widgets/tests", test_radio_group)
{
    QtWarningCheck qtWarnings;
    auto block = makeWidget("/widgets/radio_group");
    block.call("setTitle", QString("RadioGroup"));
    block.call("setOptions", makeOptions());
    block.call("setValue", Pothos::Object(std::string("b")));
    processEvents();
    qtWarnings.check();
    POTHOS_TEST_EQUAL(block.call<Pothos::Object>("value").convert<std::string>(), "b");
}

POTHOS_TEST_BLOCK("/widgets/tests", test_text_entry)
{
    QtWarningCheck qtWarnings;
    auto block = makeWidget("/widgets/text_entry");
    block.call("setTitle", QString("TextEntry"));
    block.call("setValue", QString("hello"));
    processEvents();
    qtWarnings.check();
    POTHOS_TEST_TRUE(block.call<QString>("value") == "hello");
}

POTHOS_TEST_BLOCK("/widgets/tests", test_push_button)
{
    QtWarningCheck qtWarnings;
    auto block = makeWidget("/widgets/push_button");
    block.call("setTitle", QString("Push"));
    Pothos::ObjectVector args;
    args.emplace_back(Pothos::Object(1));
    args.emplace_back(Pothos::Object(std::string("x")));
    block.call("setArgs", args);
    processEvents();
    qtWarnings.check();
    POTHOS_TEST_EQUAL(block.call<Pothos::ObjectVector>("getArgs").size(), 2);
}

POTHOS_TEST_BLOCK("/widgets/tests", test_chat_box)
{
    QtWarningCheck qtWarnings;
    auto block = makeWidget("/widgets/chat_box");
    block.call("setUsername", QString("user"));
    block.call("setColor", QString("blue"));
    processEvents();
    qtWarnings.check();
}

POTHOS_TEST_BLOCK("/widgets/tests", test_colored_box)
{
    QtWarningCheck qtWarnings;
    auto block = makeWidget("/widgets/colored_box");
    block.call("setTitle", QString("ColoredBox"));
    block.call("setFontSize", 14);
    block.call("setText", QString("text"));
    block.call("setColor", QString("red"));
    processEvents();
    qtWarnings.check();
}

POTHOS_TEST_BLOCK("/widgets/tests", test_text_display)
{
    QtWarningCheck qtWarnings;
    auto block = makeWidget("/widgets/text_display");
    block.call("setTitle", QString("TextDisplay"));
    block.call("setValue", Pothos::Object(42));
    processEvents();
    qtWarnings.check();
}
