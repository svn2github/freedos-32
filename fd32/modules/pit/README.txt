
This module uses the PIT timer and, if so configured, the Time Stamp Counter
(TSC). It operates in a legacy compatible mode by default, which programs the
PIT in the old DOS way, with timer interrupt ca. every 54.925ms, and makes
no assumption about the state of the PIT after initialization. Use compatible
mode if you are going to run DPMI or 16-bit DOS applications that reads or
reprograms the timer. Native mode allows better resolution/latency than
compatible mode.

Options:
--no-export
    The syscall_table will not be modified if this option is used.

--tsc-delay
    Use the TSC to get a more accurate nano_delay.

--tsc-time
    Use the TSC in the time event dispatcher.
    WARNING: This option and the previous should only be used on systems that
are known to have a TSC (Pentium+), because the CPUID identification which
detects the presence of the TSC is not yet implemented.

--tick-period=<T>
    PIT will be programed to generate an interrupt (tick) every T
microseconds. This option elables native mode.

--max-count=<n>
    Specify the max timer value (in timer mode 2) directly. The resulting tick
frequency is (nominally) 1193181.66667/n Hz. This option elables native mode.

TODO:
-CPUID for the TSC.
-Sort the time events to reduce. Searching through the whole list every tick
takes too much time at higher tick frequencies.
-Enable programming of one of the other timers to increase reliability at high
tick frequency as an alternative to the TSC.
-Time of day at 10ms resolution (or better).
-Let pit_gettime return *nominal* latency for each timer.
