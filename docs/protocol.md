# Serial protocol

The timer shows up as a USB serial port (a CDC device named "pinewood-time race timer") and a USB
keyboard (used only by the reset button's [next heat](#the-reset-button-next-heat) mode). The port
settings don't matter over USB, but software that asks can use 9600 baud, 8 data bits, no parity, 1 stop
bit. Opening the port at 1200 baud restarts the Pico ready for new firmware.

It speaks the protocol of the PDT (Pinewood Derby Timer) by David Gadberry, so race software with a PDT
option should be able to drive it. This firmware is a separate implementation written from the protocol's
behaviour; it contains none of the PDT's code.

Every message from the timer ends with `\r\n`.

## The timer says

| Message | When |
|---|---|
| `P` | Powered on. It then waits for a reset (`R` or the reset button). |
| `K` | Reset and ready: the next time the gate opens, a race starts. |
| `O` | Asked to get ready, but the start gate is already open. Close it and reset again. |
| `B` | The gate opened: the race has started. |
| `.` | A command was done. |
| results | When every lane is in (or the race is ended early): one line per lane, lane 1 first. |
| `#next` | The reset button was pressed in [next heat](#the-reset-button-next-heat) mode. Not part of the PDT protocol. |

A results line is the lane, ` - `, then the time in seconds to 4 places: `1 - 2.8734`. A lane with no time
(masked, or not in when the race was ended) reads `99.9990`.

```
B
1 - 2.8734
2 - 2.9105
3 - 99.9990
4 - 2.8811
```

## Commands

Single characters, no line ending needed.

| Send | Does |
|---|---|
| `R` | Reset: get ready for the next race (`K`, or `O` if the gate is open). During a race it ends the race and sends the results; send `R` again to get ready. |
| `F` | End the race now and send the results (lanes not in get `99.9990`). |
| `Q` | Send the last results again. |
| `G` | Is the gate open? `O` if so, `.` if not. |
| `M` then a lane digit | Leave that lane out of the next races (`M3`). |
| `U` | Put every lane back in. |
| `N` | Number of lanes: `numl=4`. |
| `V` | Protocol version: `vert=3.40`. |
| `I` | Information: firmware version and settings. |
| `T` | Each lane input's raw level, lane 1 first: `lanes 0010`. |
| `D` | Debug messages on or off. |
| `S`, `X` | Accepted for compatibility (start solenoid, display multiplexer); this board has neither. |

## Settings

Lines starting with `#`, ended with Enter (`\r` or `\n`). They apply straight away; `#save` keeps them
across power cycles (in the Pico's flash).

| Send | Does |
|---|---|
| `#lanes 6` | Lanes in use, 1 to 8 (default 4). |
| `#lane-trip high` / `low` | Whether a lane input goes high (default, for break-beam receivers) or low when a car crosses. |
| `#gate-trip high` / `low` | Whether the gate input goes high (default: a switch held closed by the shut gate, which opens as the gate drops) or low when the gate opens. |
| `#brightness 6` | Lane display brightness, 0 to 15. |
| `#button reset` / `next` | What the reset button does when there's no race running (default `reset`); see below. |
| `#button-key enter` | The key the button types in `next` mode (default `enter`; `none` for no key). |
| `#save` | Save the settings. Replies `saved`. |
| `#info` | The same as `I`. |

A setting replies `.` when accepted, `?` when not understood. Settings are case-insensitive.

## The reset button: next heat

With `#button reset` (the default) the reset button gets the timer ready for the next race, the same as
`R`. Race software sends `R` itself before each heat, though, so with software connected the button can
do something more useful: `#button next` makes it ask the software for the next heat. Each press:

- sends `#next` on the serial port, for software that listens for it, and
- types a key on the timer's USB keyboard, for software with a keyboard shortcut for the next heat. Set
  it with `#button-key`: `enter`, `space`, `tab`, `escape`, `pageup`, `pagedown`, `up`, `down`, `left`,
  `right`, `home`, `end`, `f1` to `f12`, a letter or digit (`n`), or `none`. The race software's window
  needs to be the one in front for the key to reach it.

In either mode, pressing it during a race ends the race (for a car stuck on the track), holding it at
power-on starts test mode, and pressing it in test mode leaves test mode. If no software has the port
open, `next` falls back to a reset, so the button still does something useful on its own.

```
#button next
#button-key pagedown
#save
```

## Test mode

Hold the reset button while plugging in. The status light goes white, the timer sends
`test lanes=.1.. gate=closed` four times a second (`1` = that lane's beam is broken), and each lane display
shows `8888` while its beam is broken. Press reset to leave.
