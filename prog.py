#!/usr/bin/env python
import argparse, enum, os, select, termios


class record_type(enum.IntEnum):
    data = 0
    end_of_file = 1
    extended_segment_address = 2
    start_segment_address = 3
    extended_linear_address = 4
    start_linear_address = 5


class record(object):
    @staticmethod
    def parse_hex(filename):
        f = open(filename, "r")
        buf = f.read()
        f.close()
        return [record(l) for l in buf.splitlines()]

    def __init__(self, buf = None, **fields):
        if buf is not None:
            self.addr = int(buf[3:7], 0x10)
            self.rec_type = record_type(int(buf[7:9], 0x10))
            self.data = [int(buf[x:x + 2], 0x10)
                    for x in range(9, len(buf) - 2, 2)]
        else:
            self.addr = 0
            self.rec_type = record_type.data
            self.data = []
        for field, value in fields.items():
            if getattr(self, field, None) is not None:
                setattr(self, field, value)

    def __str__(self):
        buf = ":%02X%04X%02X" % (len(self.data), self.addr, self.rec_type)
        checksum = len(self.data) + (self.addr >> 8) + self.addr + self.rec_type
        for d in self.data:
            buf += "%02X" % d
            checksum += d
        return buf + "%02X" % (-checksum & 0xff)

    def __bytes__(self):
        return bytes(str(self), "UTF-8")


class target(object):
    targets = None
    @staticmethod
    def factory(name, *args):
        return target.targets[name](*args)

    def __init__(self, filename):
        tty = os.open(filename, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        attr = termios.tcgetattr(tty)
        termios.tcsetattr(tty, termios.TCSANOW, [
            termios.BRKINT | termios.IGNPAR | termios.IXON,  # iflag
            0,  # oflag
            termios.CS8 | termios.CREAD | termios.CLOCAL,  # cflag
            termios.IEXTEN | termios.ECHOE | termios.ECHOK,  # lflag
            termios.B19200,  # ospeed
            termios.B19200,  # ispeed
            attr[6],  # cc
        ])
        self.tty = tty

    def send_record(self, rec):
        select.select((), (self.tty,), ())
        os.write(self.tty, bytes(rec))
        buf = b""
        while True:
            select.select((self.tty,), (), ())
            buf += os.read(self.tty, 1)
            if buf[-1:] not in b"\n\r0123456789ABCDEFabcdef:":
                break
        assert buf[-1:] == b".",\
            "error writing \"%(record)s\": 0x%(code)02x ('%(code)c')" % {
                    "code": buf[-1],
                    "record": rec,
            }

    def send_hex(self, records):
        for rec in records:
            self.send_record(rec)
            if rec.rec_type == record_type.end_of_file:
                break


class mcs51(target):
    def __init__(self, filename):
        super().__init__(filename)
        u = b"U"
        while True:
            os.write(self.tty, u)
            select.select((self.tty,), (), (), 0.04)
            try:
                c = os.read(self.tty, 1)
            except BlockingIOError:
                continue
            if c == u:
                break

    def send_hex(self, records):
        erased = [False] * 0x200  # 0x10000 bytes / 0x80 bytes-per-sector
        ins = 0
        for rec in records:
            sector = int(rec.addr / 0x80)
            if rec.rec_type == record_type.data and not erased[sector]:
                erased[sector] = True
                addr = sector * 0x80
                records.insert(ins, record(
                    addr = 0,
                    rec_type = 3,
                    data = [8, addr >> 8 & 0xff, addr & 0xff],
                ))
                ins += 1
        super().send_hex(records)


class avr(target):
    def __init__(self, filename):
        super().__init__(filename)
        self.prompt()
        os.write(self.tty, b"reset prog\r")

    def prompt(self):
        count, buf = 0, b""
        while buf != b"> ":
            r, w, e = select.select((self.tty,), (), (), 0.08)
            if not r and not w and not e:
                count += 1
                assert count < 8, "couldn't get a prompt"
                os.write(self.tty, b"\r")
                continue
            buf = buf[-1:] + os.read(self.tty, 1)

    def send_record(self, record):
        self.prompt()
        super().send_record(record)

    def send_hex(self, records):
        self.prompt()
        os.write(self.tty, b"erase\ry")
        super().send_hex(records)
        os.write(self.tty, b"reset\r")


target.targets = {
        "8051": mcs51,
        "avr": avr,
}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
            description = "A simple serial flash programmer.",
    )
    parser.add_argument("image",
            help = "the program image to be flashed (in Intel hex format)",
    )
    parser.add_argument("ttyS",
            help = "serial device to which the program image shall be written",
    )
    parser.add_argument("-t", "--target",
            default = "avr",
            help = "target architecture",
            metavar = "TARG",
    )
    args = parser.parse_args()
    records = record.parse_hex(args.image)
    targ = target.factory(args.target, args.ttyS)
    targ.send_hex(records)
