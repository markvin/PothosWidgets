// Copyright (c) 2026 Pothos contributors
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>
#include <Pothos/Object/Containers.hpp>
#include <QApplication>
#include <QCoreApplication>
#include <QString>
#include <QWidget>
#include <memory>
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
        POTHOS_TEST_CLOSE(block.call<double>("value"), 5.0, 1e-9);
    }
}

POTHOS_TEST_BLOCK("/widgets/tests", test_numeric_entry)
{
    auto block = makeWidget("/widgets/numeric_entry");
    block.call("setTitle", QString("Entry"));
    block.call("setMinimum", -10.0);
    block.call("setMaximum", 10.0);
    block.call("setDecimals", 2);
    block.call("setValue", 7.25);
    processEvents();
    POTHOS_TEST_CLOSE(block.call<double>("value"), 7.25, 1e-9);
}

POTHOS_TEST_BLOCK("/widgets/tests", test_odometer)
{
    auto block = makeWidget("/widgets/odometer");
    block.call("setTitle", QString("Odometer"));
    block.call("setValueMin", qint64(0));
    block.call("setValueMax", qint64(99999));
    block.call("setValue", qint64(1234));
    processEvents();
    POTHOS_TEST_EQUAL(block.call<qint64>("value"), 1234);
}

POTHOS_TEST_BLOCK("/widgets/tests", test_planar_select)
{
    auto block = makeWidget("/widgets/planar_select");
    block.call("setTitle", QString("Planar"));
    block.call("setMinimum", std::vector<double>{-1.0, -1.0});
    block.call("setMaximum", std::vector<double>{1.0, 1.0});
    block.call("setValue", std::vector<double>{0.25, -0.5});
    processEvents();
    POTHOS_TEST_EQUALV(block.call<std::vector<double>>("value"), (std::vector<double>{0.25, -0.5}));
}

POTHOS_TEST_BLOCK("/widgets/tests", test_drop_down)
{
    auto block = makeWidget("/widgets/drop_down");
    block.call("setTitle", QString("DropDown"));
    block.call("setOptions", makeOptions());
    block.call("setValue", Pothos::Object(std::string("b")));
    processEvents();
    POTHOS_TEST_EQUAL(block.call<Pothos::Object>("value").convert<std::string>(), "b");
}

POTHOS_TEST_BLOCK("/widgets/tests", test_radio_group)
{
    auto block = makeWidget("/widgets/radio_group");
    block.call("setTitle", QString("RadioGroup"));
    block.call("setOptions", makeOptions());
    block.call("setValue", Pothos::Object(std::string("b")));
    processEvents();
    POTHOS_TEST_EQUAL(block.call<Pothos::Object>("value").convert<std::string>(), "b");
}

POTHOS_TEST_BLOCK("/widgets/tests", test_text_entry)
{
    auto block = makeWidget("/widgets/text_entry");
    block.call("setTitle", QString("TextEntry"));
    block.call("setValue", QString("hello"));
    processEvents();
    POTHOS_TEST_TRUE(block.call<QString>("value") == "hello");
}

POTHOS_TEST_BLOCK("/widgets/tests", test_push_button)
{
    auto block = makeWidget("/widgets/push_button");
    block.call("setTitle", QString("Push"));
    Pothos::ObjectVector args;
    args.emplace_back(Pothos::Object(1));
    args.emplace_back(Pothos::Object(std::string("x")));
    block.call("setArgs", args);
    processEvents();
    POTHOS_TEST_EQUAL(block.call<Pothos::ObjectVector>("getArgs").size(), 2);
}

POTHOS_TEST_BLOCK("/widgets/tests", test_chat_box)
{
    auto block = makeWidget("/widgets/chat_box");
    block.call("setUsername", QString("user"));
    block.call("setColor", QString("blue"));
    processEvents();
}

POTHOS_TEST_BLOCK("/widgets/tests", test_colored_box)
{
    auto block = makeWidget("/widgets/colored_box");
    block.call("setTitle", QString("ColoredBox"));
    block.call("setFontSize", 14);
    block.call("setText", QString("text"));
    block.call("setColor", QString("red"));
    processEvents();
}

POTHOS_TEST_BLOCK("/widgets/tests", test_text_display)
{
    auto block = makeWidget("/widgets/text_display");
    block.call("setTitle", QString("TextDisplay"));
    block.call("setValue", Pothos::Object(42));
    processEvents();
}
