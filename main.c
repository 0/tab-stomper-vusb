#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>

#include <util/delay.h>

#include "usbdrv.h"

/*
 * At 16 MHz, with the prescaler set to 1024, the 8-bit timer overflows each
 *
 *   1 / (16 MHz / 1024 / 256) =~ 16.384 ms.
 */

// More than 163 ms, which should be plenty.
#define DEBOUNCE_WAIT_TOTAL 10
// USB uses units of 4 ms for the idle rate, but our timer is slower than that
// by a factor of about ceil(16.384 ms / 4 ms) = 5.
#define IDLE_RATE_STEP 5

#define UP_ARROW_NUM 2

#define BUTTON_PIN 5

const char usbHidReportDescriptor[23] PROGMEM = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x06, // USAGE (Keyboard)
    0xa1, 0x01, // COLLECTION (Application)
    0x05, 0x07, //  USAGE_PAGE (Keyboard)
    0x19, 0x00, //  USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x52, //  USAGE_MAXIMUM (Keyboard UpArrow)
    0x15, 0x00, //  LOGICAL_MINIMUM (0)
    0x25, 0x52, //  LOGICAL_MAXIMUM (82)
    0x75, 0x08, //  REPORT_SIZE (8)
    0x95, 0x01, //  REPORT_COUNT (1)
    0x81, 0x00, //  INPUT (Data,Ary,Abs)
    0xc0,       // END_COLLECTION
};

static uchar idle_rate;
static uchar current_key;

static void set_key(uchar key) {
    switch (key) {
        case 0:
            current_key = 0x00;
            break;
        case 1:
            // PageDown
            current_key = 0x4e;
            break;
        case 2:
            // UpArrow
            current_key = 0x52;
            break;
    }
}

uchar usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;

    if ((rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_CLASS) {
        return 0;
    }

    switch (rq->bRequest) {
        case USBRQ_HID_GET_REPORT:
            usbMsgPtr = &current_key;
            return 1;
        case USBRQ_HID_GET_IDLE:
            usbMsgPtr = &idle_rate;
            return 1;
        case USBRQ_HID_SET_IDLE:
            idle_rate = rq->wValue.bytes[1];
            break;
    }

    return 0;
}

static uchar get_button() {
    return !(PIND & _BV(BUTTON_PIN));
}

int main(void) {
    uchar i;

    // Whether we have a new report to send.
    uchar send_report = 0;
    // Whether we must send a report this time around.
    uchar force_report = 0;
    // State machine state.
    uchar state = 0;
    // How long we need to wait until the next forced report.
    uchar idle_wait = 0;
    // How many more times the up arrow needs to be sent.
    uchar up_arrow_left = 0;
    // How long is left until we're done debouncing.
    uchar debounce_wait = 0;

    wdt_enable(WDTO_1S);

    // Button and USB pins are inputs.
    DDRD = ~(_BV(BUTTON_PIN) | USBMASK);
    // Button gets a pullup.
    PORTD = _BV(BUTTON_PIN);

    // Force enumeration.
    usbDeviceDisconnect();
    i = 0;
    while (--i) {
        wdt_reset();
        _delay_ms(2);
    }
    usbDeviceConnect();

    // 1/1024
    TCCR0B = _BV(CS02) | _BV(CS00);

    // Enable USB and interrupts.
    usbInit();
    sei();

    for (;;) {
        // Maintenance.
        wdt_reset();
        usbPoll();

        // Timer.
        if (TIFR0 & (1 << TOV0)) {
            // Restart the timer.
            TIFR0 = 1 << TOV0;

            if (debounce_wait > 0) {
                debounce_wait--;
            }

            // An idle rate of zero means wait forever.
            if (idle_rate != 0) {
                if (idle_wait >= IDLE_RATE_STEP) {
                    idle_wait -= IDLE_RATE_STEP;
                } else {
                    idle_wait = idle_rate;
                    force_report = 1;
                }
            }
        }

        // State machine for the output logic. Don't do anything if we still
        // haven't sent the last report.
        if (!send_report) {
            switch (state) {
                case 0:
                    // Waiting for the button to be pressed.
                    if (debounce_wait == 0 && get_button()) {
                        // Press PageDown.
                        state = 1;
                        set_key(1);
                        send_report = 1;
                    }
                    break;
                case 1:
                    // Pressing PageDown.
                    if (current_key != 0x00) {
                        // Release PageDown.
                        set_key(0);
                        send_report = 1;
                    } else {
                        // Press UpArrow.
                        state = 2;
                        set_key(2);
                        up_arrow_left = UP_ARROW_NUM - 1;
                        send_report = 1;
                    }
                    break;
                case 2:
                    // Pressing and releasing UpArrow several times.
                    if (current_key != 0x00) {
                        // Release UpArrow.
                        set_key(0);
                        send_report = 1;
                    } else {
                        if (up_arrow_left > 0) {
                            // Press UpArrow.
                            set_key(2);
                            up_arrow_left--;
                            send_report = 1;
                        } else {
                            // Done.
                            state = 3;
                        }
                    }
                    break;
                case 3:
                    // Waiting for the button to be released.
                    if (!get_button()) {
                        // Start debounce wait.
                        state = 0;
                        debounce_wait = DEBOUNCE_WAIT_TOTAL;
                    }
                    break;
            }
        }

        if ((send_report || force_report) && usbInterruptIsReady()) {
            send_report = 0;
            force_report = 0;

            usbSetInterrupt(&current_key, 1);
        }
    }

    return 0;
}
