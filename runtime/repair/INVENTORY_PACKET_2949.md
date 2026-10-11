# Repair 0.23.0 – original inventory package serializer

Status: 23.09.2026, Steam 25455892 / EXE 1.0.0.2949. development module; the desktop app remains v0.5.9 and does not yet offer a free repair. These samples only read the pinned EXE and perform functional copies on private data. The separately commissioned offline inventory order is not one of them.

## What has been added

The original transmitter `0x29ec2e0` and its stream selection `0x2961c30` run in the private test host. Ten cases connect the transmitter with repair planning, native post-copies, field recorders, original client alignment and new protected capture. The connection between transmitter and Ack consists of its own limited decoder and test receivers. It is not a real engine transport and not a proof of storage.

The transmitter generates exactly the following user data in four write calls:

|Offset|Size size|Content|
|---|---:|---|
|0|4 Byte|Actor identifier from `actor+0x60`, for zero actor 0|
|4|2 Byte|Translated inventory key|
|6|2 Byte|Slot|
|8|2 Byte|Absolute Main Durability|

The packet header receives `uint16_t 0x0c02` and then byte `0xff`. The user data contains **neither item UID nor base data nor task identifier**. The own identity, socket and order checks therefore remain necessary. The previous client sample is documented under [Inventar-Ack](INVENTORY_EVENTS_2949.md). The original outer packet reader is still not executed; his external success status swallows the internal error and is not enough as proof.

The stream selection only reads its own TLS object with 0x300 byte. At `TLS+0x250` is its private context. The transmitter first emptys the receiver length at `context+0x2b8`; The virtual descriptor method fills it. Without a receiver, he returns. The selection shall take into account the activation value, channel types and virtual descriptor comparison; an unsuitable first channel does not prevent the choice of a suitable second channel.

## Errors and remaining limits

- Without a recipient, there is no package. A missing stream buffer only generates
a diagnosis. The return of the sender is `void` and does not contain a receipt.
- Even if the private writer does not accept the bytes, the original code
head and call for the finalization. Such a call is not a delivery document.
- Without a suitable stream, the original pool branch is reached. Here only
the case of a failed private allocation is executed. Successful pool allocation, serializer, transport, reference delivery and release of this branch are still open. Both TLS modes are therefore not a test of both pool release paths.
- Receiver setup, descriptor comparison, stream buffer, byte receiver, finalisation
Diagnosis is your own callback. Their lifetime and thread binding in the current game are not occupied.
- Server marking and regular storage remain test receivers. The Ten
Integrated cases still do not prove a complete server repair.
- Missing or duplicate delivery, incorrect actor, error in the inner acceleration and
UID change after the field step does not result in success. The error lock prevents automatic repetition after a possible partial change.

## Pinned functions and testing

EXE-SHA-256: `a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a`.

|Function|RVA/Length|SHA-256|
|---|---|---|
|Package serializers|`0x29ec2e0` / `0x4c0`|`b227df1159b017f454a9362c085861f99697e526398de71a3f1fc738cf039d7f`|
|Stream selection|`0x2961c30` / `0xab`|`98a78f3849be23c8fdf87be14801564a00b6f54f0438c40140a5130e5bd0842b`|

Both complete functional limits are checked by pdata. Two TLS offices are redirected to the private context; All other codebytes remain the same. The serializer retains its stack wind data, but does not use a voice cleanuphandler from the game. Private code ranges are RX, data R-only.

**36 new cases:** 13 direct scenarios in two TLS modes and ten integrations. Directly checked: exact field widths/byte sequence, zero actuator, zero/maximum values, no/multiple receivers, missing buffer, discarded bytes, disabled stream selection, type deviation, negative comparison and selection of the second channel. Integrations: ordinary repair, damaged base only, no wear, missing receiver/buffer, incorrect container translation, missing/double delivery, UID change and false actor identifier.

MSVC Release `/W4 /WX` and nine CTest suites passed. Overall: **454 native scenarios, 4,812 counted calls, 206,786 conditions**. Many conditions are repeated byte checks and not independent cases. 102 code ranges, 127 pdata fragments, 60 replaced TLS locations as well as an unchanged stack sample access to your own Windows thread. CTest conditions: 11/1,529/519/25/55/612/29/202/273.

The next practical gaps remain the actual server/memory termination, player and manager lifetime, engine thread/queue, input, loader and B0.
