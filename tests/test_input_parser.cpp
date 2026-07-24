#include <QtTest/QtTest>
#include <rapidcheck.h>

#include <QtTUI/InputParser.h>
#include <QtTUI/TuiEvents.h>

using namespace QtTUI;

// Helper: collects parsed events via signals (avoids needing event loop processing)
class EventCollector : public QObject {
    Q_OBJECT
public:
    struct KeyRecord {
        int key;
        Qt::KeyboardModifiers modifiers;
        QString text;
    };
    struct MouseRecord {
        int button;
        int x, y;
        int eventType;
    };
    struct ResizeRecord {
        int cols, rows;
    };

    QList<KeyRecord> keys;
    QList<MouseRecord> mice;
    QList<ResizeRecord> resizes;

    void connectTo(InputParser *parser)
    {
        connect(parser, &InputParser::keyEventParsed,
                this, [this](int key, Qt::KeyboardModifiers mods, const QString &text) {
                    keys.append({key, mods, text});
                });
        connect(parser, &InputParser::mouseEventParsed,
                this, [this](int button, int x, int y, int eventType) {
                    mice.append({button, x, y, eventType});
                });
        connect(parser, &InputParser::resizeEventParsed,
                this, [this](int cols, int rows) {
                    resizes.append({cols, rows});
                });
    }

    void clear()
    {
        keys.clear();
        mice.clear();
        resizes.clear();
    }
};

class TestInputParser : public QObject {
    Q_OBJECT

private slots:
    // Property 1: Printable character to key event mapping
    void property1_printableCharToKeyEvent();

    // Property 2: Modifier key preservation
    void property2_modifierKeyPreservation();

    // Property 3: SGR mouse event round-trip
    void property3_sgrMouseRoundTrip();

    // Property 4: Resize event generation
    void property4_resizeEventGeneration();

    // Unit tests for specific known sequences
    void testArrowKeys();
    void testFunctionKeys();
    void testHomeEndInsertDelete();
    void testPageUpDown();
    void testCtrlKeys();
    void testAltKeys();
    void testBackspace();
    void testEscapeKey();
    void testBacktab();
    void testLoneEscapeTimeout();
    void testEscapeSequenceNotBrokenByTimeout();
    void testCtrlPunctuation();
    void testMouseWheel();
    void testMouseModifiers();
    void testNonBmpUtf8();
    void testBracketedPaste();
    void testKittyKeyboardProtocol();
};

// Helper: captures posted TuiMouseEvents (fields the signal doesn't carry)
class MouseEventReceiver : public QObject {
public:
    struct Rec {
        int type;
        Qt::MouseButton button;
        QPoint pos;
        Qt::KeyboardModifiers modifiers;
        int wheelDelta;
    };
    QList<Rec> recs;

protected:
    bool event(QEvent *e) override
    {
        if (e->type() == TuiEventTypes::mouseEventType()) {
            auto *me = static_cast<TuiMouseEvent *>(e);
            recs.append({me->mouseType(), me->button(), me->pos(),
                         me->modifiers(), me->wheelDelta()});
            return true;
        }
        return QObject::event(e);
    }
};

// Feature: qttui-framework, Property 1: Printable character to QKeyEvent mapping
void TestInputParser::property1_printableCharToKeyEvent()
{
    rc::check("For any printable ASCII char, InputParser produces key event with correct key code",
        []() {
            // Generate printable ASCII: 0x20-0x7E
            const char ch = *rc::gen::inRange(0x20, 0x7F);

            InputParser parser(nullptr);
            EventCollector collector;
            collector.connectTo(&parser);

            QByteArray input;
            input.append(ch);
            parser.processInput(input);

            RC_ASSERT(collector.keys.size() == 1);

            const auto &key = collector.keys.first();
            QChar qch(ch);
            int expectedKey = qch.toUpper().unicode();
            RC_ASSERT(key.key == expectedKey);
            RC_ASSERT(key.text == QString(qch));

            // Uppercase letters should have Shift modifier
            if (qch.isUpper()) {
                RC_ASSERT(key.modifiers & Qt::ShiftModifier);
            } else {
                RC_ASSERT(!(key.modifiers & Qt::ShiftModifier));
            }
        });
}

// Feature: qttui-framework, Property 2: Modifier key preservation
void TestInputParser::property2_modifierKeyPreservation()
{
    rc::check("For any key + modifier subset, the resulting event contains exactly those modifiers",
        []() {
            // Test xterm-style modifier encoding in CSI sequences
            // CSI 1;{mod+1} {A-D} for arrow keys with modifiers
            // mod bits: 1=Shift, 2=Alt, 4=Ctrl
            const int modBits = *rc::gen::inRange(0, 8); // 0-7: all combinations of Shift/Alt/Ctrl
            // Pick an arrow key final byte
            const char arrows[] = {'A', 'B', 'C', 'D'};
            const int arrowIdx = *rc::gen::inRange(0, 4);
            const char arrowChar = arrows[arrowIdx];

            Qt::KeyboardModifiers expectedMods = Qt::NoModifier;
            if (modBits & 1) expectedMods |= Qt::ShiftModifier;
            if (modBits & 2) expectedMods |= Qt::AltModifier;
            if (modBits & 4) expectedMods |= Qt::ControlModifier;

            int modParam = modBits + 1; // xterm encoding: modifier bits + 1

            InputParser parser(nullptr);
            EventCollector collector;
            collector.connectTo(&parser);

            // Build CSI sequence: \e[1;{modParam}{arrowChar}
            QByteArray input;
            input.append('\x1b');
            input.append('[');
            input.append('1');
            input.append(';');
            input.append(QByteArray::number(modParam));
            input.append(arrowChar);

            parser.processInput(input);

            RC_ASSERT(collector.keys.size() == 1);
            RC_ASSERT(collector.keys.first().modifiers == expectedMods);
        });
}

// Feature: qttui-framework, Property 3: SGR mouse event round-trip
void TestInputParser::property3_sgrMouseRoundTrip()
{
    rc::check("Encoding mouse params as SGR and feeding to InputParser produces matching event",
        []() {
            // Generate mouse parameters
            const int btnId = *rc::gen::inRange(0, 3); // 0=left, 1=middle, 2=right
            const int termW = *rc::gen::inRange(10, 200);
            const int termH = *rc::gen::inRange(10, 200);
            // SGR coords are 1-based
            const int x1based = *rc::gen::inRange(1, termW + 1);
            const int y1based = *rc::gen::inRange(1, termH + 1);
            const bool isPress = *rc::gen::arbitrary<bool>();

            InputParser parser(nullptr);
            parser.setTerminalSize(QSize(termW, termH));
            EventCollector collector;
            collector.connectTo(&parser);

            // Build SGR mouse sequence: \e[<Btn;X;Y{M|m}
            QByteArray input;
            input.append('\x1b');
            input.append('[');
            input.append('<');
            input.append(QByteArray::number(btnId));
            input.append(';');
            input.append(QByteArray::number(x1based));
            input.append(';');
            input.append(QByteArray::number(y1based));
            input.append(isPress ? 'M' : 'm');

            parser.processInput(input);

            RC_ASSERT(collector.mice.size() == 1);

            const auto &mouse = collector.mice.first();

            // Check button
            Qt::MouseButton expectedBtn = Qt::NoButton;
            if (btnId == 0) expectedBtn = Qt::LeftButton;
            else if (btnId == 1) expectedBtn = Qt::MiddleButton;
            else if (btnId == 2) expectedBtn = Qt::RightButton;
            RC_ASSERT(mouse.button == static_cast<int>(expectedBtn));

            // Check position (0-based, clamped)
            int expectedX = std::clamp(x1based - 1, 0, termW - 1);
            int expectedY = std::clamp(y1based - 1, 0, termH - 1);
            RC_ASSERT(mouse.x == expectedX);
            RC_ASSERT(mouse.y == expectedY);

            // Check event type
            int expectedType = isPress
                ? static_cast<int>(TuiMouseEvent::Press)
                : static_cast<int>(TuiMouseEvent::Release);
            RC_ASSERT(mouse.eventType == expectedType);
        });
}

// Feature: qttui-framework, Property 4: Resize event generation
void TestInputParser::property4_resizeEventGeneration()
{
    rc::check("For any valid (cols, rows), resize notification produces matching resize event",
        []() {
            const int cols = *rc::gen::inRange(1, 500);
            const int rows = *rc::gen::inRange(1, 500);

            InputParser parser(nullptr);
            EventCollector collector;
            collector.connectTo(&parser);

            parser.processResize(QSize(cols, rows));

            RC_ASSERT(collector.resizes.size() == 1);
            RC_ASSERT(collector.resizes.first().cols == cols);
            RC_ASSERT(collector.resizes.first().rows == rows);
        });
}

void TestInputParser::testArrowKeys()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);

    // Up: \e[A
    parser.processInput(QByteArray("\x1b[A"));
    QCOMPARE(collector.keys.size(), 1);
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_Up));
    QCOMPARE(collector.keys.last().modifiers, Qt::NoModifier);

    // Down: \e[B
    parser.processInput(QByteArray("\x1b[B"));
    QCOMPARE(collector.keys.size(), 2);
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_Down));

    // Right: \e[C
    parser.processInput(QByteArray("\x1b[C"));
    QCOMPARE(collector.keys.size(), 3);
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_Right));

    // Left: \e[D
    parser.processInput(QByteArray("\x1b[D"));
    QCOMPARE(collector.keys.size(), 4);
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_Left));
}

void TestInputParser::testFunctionKeys()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);

    // F1 via SS3: \eOP
    parser.processInput(QByteArray("\x1bOP"));
    QCOMPARE(collector.keys.size(), 1);
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_F1));

    // F2 via SS3: \eOQ
    parser.processInput(QByteArray("\x1bOQ"));
    QCOMPARE(collector.keys.size(), 2);
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_F2));

    // F5 via CSI: \e[15~
    parser.processInput(QByteArray("\x1b[15~"));
    QCOMPARE(collector.keys.size(), 3);
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_F5));

    // F12 via CSI: \e[24~
    parser.processInput(QByteArray("\x1b[24~"));
    QCOMPARE(collector.keys.size(), 4);
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_F12));
}

void TestInputParser::testHomeEndInsertDelete()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);

    // Home: \e[H
    parser.processInput(QByteArray("\x1b[H"));
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_Home));

    // End: \e[F
    parser.processInput(QByteArray("\x1b[F"));
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_End));

    // Insert: \e[2~
    parser.processInput(QByteArray("\x1b[2~"));
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_Insert));

    // Delete: \e[3~
    parser.processInput(QByteArray("\x1b[3~"));
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_Delete));
}

void TestInputParser::testPageUpDown()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);

    // PageUp: \e[5~
    parser.processInput(QByteArray("\x1b[5~"));
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_PageUp));

    // PageDown: \e[6~
    parser.processInput(QByteArray("\x1b[6~"));
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_PageDown));
}

void TestInputParser::testCtrlKeys()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);

    // Ctrl+A = 0x01
    parser.processInput(QByteArray("\x01"));
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_A));
    QCOMPARE(collector.keys.last().modifiers, Qt::ControlModifier);

    // Ctrl+C = 0x03
    parser.processInput(QByteArray("\x03"));
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_C));
    QCOMPARE(collector.keys.last().modifiers, Qt::ControlModifier);
}

void TestInputParser::testAltKeys()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);

    // Alt+a = ESC a
    parser.processInput(QByteArray("\x1b" "a"));
    QCOMPARE(collector.keys.size(), 1);
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_A));
    QVERIFY(collector.keys.last().modifiers & Qt::AltModifier);
}

void TestInputParser::testBackspace()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);

    // DEL (0x7F) = Backspace
    parser.processInput(QByteArray("\x7f"));
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_Backspace));
}

void TestInputParser::testEscapeKey()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);

    // ESC ESC -> first ESC emits Key_Escape
    parser.processInput(QByteArray("\x1b\x1b"));
    QVERIFY(collector.keys.size() >= 1);
    QCOMPARE(collector.keys.first().key, static_cast<int>(Qt::Key_Escape));
}

void TestInputParser::testBacktab()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);

    // Shift+Tab: \e[Z
    parser.processInput(QByteArray("\x1b[Z"));
    QCOMPARE(collector.keys.size(), 1);
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_Backtab));
    QCOMPARE(collector.keys.last().modifiers, Qt::ShiftModifier);
}

void TestInputParser::testLoneEscapeTimeout()
{
    InputParser parser(nullptr);
    parser.setEscapeTimeout(20);
    EventCollector collector;
    collector.connectTo(&parser);

    // A bare ESC with no continuation is delivered as Key_Escape after the timeout
    parser.processInput(QByteArray("\x1b"));
    QCOMPARE(collector.keys.size(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(collector.keys.size(), 1, 1000);
    QCOMPARE(collector.keys.first().key, static_cast<int>(Qt::Key_Escape));
    QCOMPARE(collector.keys.first().modifiers, Qt::NoModifier);

    // The next printable key must arrive as a plain key, not a spurious Alt chord
    parser.processInput(QByteArray("a"));
    QCOMPARE(collector.keys.size(), 2);
    QCOMPARE(collector.keys.last().key, static_cast<int>(Qt::Key_A));
    QCOMPARE(collector.keys.last().modifiers, Qt::NoModifier);
}

void TestInputParser::testEscapeSequenceNotBrokenByTimeout()
{
    InputParser parser(nullptr);
    parser.setEscapeTimeout(20);
    EventCollector collector;
    collector.connectTo(&parser);

    // A complete sequence in one burst cancels the pending timeout
    parser.processInput(QByteArray("\x1b[A"));
    QCOMPARE(collector.keys.size(), 1);
    QCOMPARE(collector.keys.first().key, static_cast<int>(Qt::Key_Up));

    // Wait past the timeout: no spurious Escape may appear
    QTest::qWait(60);
    QCOMPARE(collector.keys.size(), 1);
}

void TestInputParser::testCtrlPunctuation()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);

    parser.processInput(QByteArray("\x1c\x1d\x1e\x1f"));
    QCOMPARE(collector.keys.size(), 4);
    QCOMPARE(collector.keys[0].key, static_cast<int>(Qt::Key_Backslash));
    QCOMPARE(collector.keys[1].key, static_cast<int>(Qt::Key_BracketRight));
    QCOMPARE(collector.keys[2].key, static_cast<int>(Qt::Key_AsciiCircum));
    QCOMPARE(collector.keys[3].key, static_cast<int>(Qt::Key_Underscore));
    for (const auto &k : collector.keys)
        QCOMPARE(k.modifiers, Qt::ControlModifier);
}

void TestInputParser::testMouseWheel()
{
    MouseEventReceiver receiver;
    InputParser parser(&receiver);
    EventCollector collector;
    collector.connectTo(&parser);

    // Wheel up (btnCode 64) and wheel down (btnCode 65)
    parser.processInput(QByteArray("\x1b[<64;10;5M"));
    parser.processInput(QByteArray("\x1b[<65;10;5M"));
    QCoreApplication::processEvents();

    QCOMPARE(receiver.recs.size(), 2);
    QCOMPARE(receiver.recs[0].type, static_cast<int>(TuiMouseEvent::Wheel));
    QCOMPARE(receiver.recs[0].button, Qt::NoButton);
    QCOMPARE(receiver.recs[0].wheelDelta, 120);
    QCOMPARE(receiver.recs[0].pos, QPoint(9, 4));
    QCOMPARE(receiver.recs[1].type, static_cast<int>(TuiMouseEvent::Wheel));
    QCOMPARE(receiver.recs[1].wheelDelta, -120);

    // No phantom button presses may be emitted for wheel input
    for (const auto &m : collector.mice)
        QCOMPARE(m.eventType, static_cast<int>(TuiMouseEvent::Wheel));
}

void TestInputParser::testMouseModifiers()
{
    MouseEventReceiver receiver;
    InputParser parser(&receiver);

    // Ctrl (16) + left press, Shift (4) + right press
    parser.processInput(QByteArray("\x1b[<16;3;4M"));
    parser.processInput(QByteArray("\x1b[<6;3;4M"));
    QCoreApplication::processEvents();

    QCOMPARE(receiver.recs.size(), 2);
    QCOMPARE(receiver.recs[0].type, static_cast<int>(TuiMouseEvent::Press));
    QCOMPARE(receiver.recs[0].button, Qt::LeftButton);
    QCOMPARE(receiver.recs[0].modifiers, Qt::ControlModifier);
    QCOMPARE(receiver.recs[1].button, Qt::RightButton);
    QCOMPARE(receiver.recs[1].modifiers, Qt::ShiftModifier);
}

void TestInputParser::testNonBmpUtf8()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);

    // U+1F600 GRINNING FACE, 4-byte UTF-8: the key must be the code point,
    // not a lone UTF-16 high surrogate, and text must round-trip
    parser.processInput(QByteArray("\xF0\x9F\x98\x80"));
    QCOMPARE(collector.keys.size(), 1);
    QCOMPARE(collector.keys.first().key, 0x1F600);
    QCOMPARE(collector.keys.first().text, QString::fromUtf8("\xF0\x9F\x98\x80"));
}

void TestInputParser::testBracketedPaste()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);
    QStringList pastes;
    connect(&parser, &InputParser::pasteEventParsed,
            [&pastes](const QString &text) { pastes.append(text); });

    // Everything between the markers is one paste: "\r" becomes "\n",
    // and an ESC or arrow sequence inside is text, not a key.
    // The end marker arrives split across two reads.
    parser.processInput(QByteArray("\x1b[200~one\rtwo \x1b[A x"));
    parser.processInput(QByteArray("\x1b[20"));
    QVERIFY(pastes.isEmpty());
    parser.processInput(QByteArray("1~q"));

    QCOMPARE(pastes.size(), 1);
    QCOMPARE(pastes.first(), QStringLiteral("one\ntwo \x1b[A x"));
    // Only the key after the paste
    QCOMPARE(collector.keys.size(), 1);
    QCOMPARE(collector.keys.first().text, QStringLiteral("q"));
}

void TestInputParser::testKittyKeyboardProtocol()
{
    InputParser parser(nullptr);
    EventCollector collector;
    collector.connectTo(&parser);

    // Ctrl+I is no longer the same as Tab
    parser.processInput(QByteArray("\x1b[105;5u"));
    // Escape arrives at once, without the timeout
    parser.processInput(QByteArray("\x1b[27u"));
    // Shift+Tab, Alt+a, Ctrl+Shift+z
    parser.processInput(QByteArray("\x1b[9;2u\x1b[97;3u\x1b[122;6u"));
    // Caps Lock (private use code): no event
    parser.processInput(QByteArray("\x1b[57358u"));
    // Keypad Enter, keypad 7, keypad Up
    parser.processInput(QByteArray("\x1b[57414u\x1b[57406u\x1b[57419u"));

    QCOMPARE(collector.keys.size(), 8);
    QCOMPARE(collector.keys[5].key, int(Qt::Key_Enter));
    QVERIFY(collector.keys[5].modifiers & Qt::KeypadModifier);
    QCOMPARE(collector.keys[6].key, int(Qt::Key_7));
    QCOMPARE(collector.keys[6].text, QStringLiteral("7"));
    QCOMPARE(collector.keys[7].key, int(Qt::Key_Up));
    QCOMPARE(collector.keys[0].key, int(Qt::Key_I));
    QCOMPARE(collector.keys[0].modifiers, Qt::KeyboardModifiers(Qt::ControlModifier));
    QVERIFY(collector.keys[0].text.isEmpty());
    QCOMPARE(collector.keys[1].key, int(Qt::Key_Escape));
    QCOMPARE(collector.keys[2].key, int(Qt::Key_Backtab));
    QCOMPARE(collector.keys[3].key, int(Qt::Key_A));
    QCOMPARE(collector.keys[3].modifiers, Qt::KeyboardModifiers(Qt::AltModifier));
    QCOMPARE(collector.keys[4].key, int(Qt::Key_Z));
    QCOMPARE(collector.keys[4].modifiers, Qt::ControlModifier | Qt::ShiftModifier);
}

QTEST_GUILESS_MAIN(TestInputParser)
#include "test_input_parser.moc"
