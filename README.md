# Lab: Buffer Overflow Defences

A password check should run its privileged action only for `password1`.
Can a different input make it do so? Investigate first, then test which
protections change the result and which actually remove the invalid write.

Work in pairs: one person types, the other records predictions and evidence.
Swap after Step 1. Allow about 45 minutes, with class discussion between steps.
Use only the provided program and container. The “privileged action” is a print
statement, not a real privileged operation.

## Setup

Start Docker Desktop (or Docker Engine on Linux). From your host terminal:

```bash
git clone https://github.com/samerlahoud/buffer-overflow-defences.git
cd buffer-overflow-defences
docker build -t bo-defences .
docker run --rm -it bo-defences
```

You are now at a **container shell** in `/lab`. Run the remaining commands there.
The image builds natively for your machine. Do not force `--platform=linux/amd64`
on Apple silicon: emulation can prevent GDB from tracing programs.

```bash
uname -m
gcc --version
cat bo_passcheck.c
```

Keep your answers in a document **outside the container**. `--rm` removes the
container and your edits when you exit. To save a repaired source file first,
use a second host terminal: `docker ps`, then
`docker cp CONTAINER_ID:/lab/bo_fixed.c ./my-bo_fixed.c` (substitute the ID).

If you already cloned the whole course repository, enter
`Demo/buffer-overflow-defences/` and start with the two Docker commands.

## Evidence sheet

Before each experiment, write a prediction. Afterward, record **both** whether
`[Privileged action executed]` appeared and whether the program terminated
normally or crashed. An action followed by a crash still counts as an action.

| Build | Prediction | Action printed | Normal - rejected - crashed | Evidence and explanation | |
|---|---|---|---|---|---|
| Baseline, canary off | | | | |
| Canary on | | | | |
| NX observation: reuse baseline | | | | |
| PIE + OS ASLR, canary off | | | | |
| Source repair | | | | |

Record your architecture, compiler version and the exact input length too.
A compiler may arrange separate local variables differently. Your measured
layout matters more than a neighbour's magic input length.

## Step 1: Explore the bug (15 minutes)

### Establish normal behaviour

```bash
gcc -g -O0 -fno-stack-protector -fno-pie -no-pie -Wl,-z,noexecstack -o bo_passcheck bo_passcheck.c
./bo_passcheck
```

Run it separately with the correct password, a short wrong password and an
input of your choice. Type or paste at the program's prompt, then press Enter.
The linker warning about `gets` is expected: it is deliberately unsafe and was
removed from ISO C11. This lab uses the legacy symbol supplied by Ubuntu's libc.

**Q1.** Find an input that prints the privileged action without the password.
What did you try, and what happened? Keep your successful input for every build.

You can generate a string to **copy and paste**, without piping it into the program:

```bash
python3 -c "print('A' * int(input('How many bytes? ')))"
```

### Inspect your explanation in GDB

```bash
gdb -q ./bo_passcheck
```

At the **`(gdb)` prompt**, enter the commands below without the prompt label:

```text
set disable-randomization off
break bo_passcheck.c:17
run
```

Type your input when the program asks. The breakpoint is on the line marked
`INSPECT`, **after** `gets` returns and **before** the password comparison.
If you edit the source, use `list main` to find that line again.

```text
print &password
print &passcheck
print (long)&passcheck - (long)&password
print/x passcheck
x/16xb password
x/4xb &passcheck
continue
```

Use `run` for another input, or `quit` to return to the shell. `continue` lets
the program finish so you can see whether it later crashes.

**Q2.** Record the two addresses and their signed byte difference. Is the flag
in the direction that successive writes from `password[0]` travel? Does the
flag change *before* the password comparison? Which bytes support your answer?

**Q3.** Find the shortest run of `A` characters that triggers the action on your
build. Also try 15, 16 and one byte fewer than your successful length. Explain
why the first out-of-bounds write and the first successful bypass can occur at
different lengths. Include the terminating `\0` that `gets` writes.

Longer inputs can crash or produce other results. Once the write leaves the
array, C gives no guarantee about the outcome. If no bypass appears, inspect
the layout and record that result. Avoid spending the whole activity guessing.

<details>
<summary>Hint after a few attempts</summary>

Compare the buffer capacity with the number of input bytes **plus one**. Then
compare the buffer's address with the flag's address. Padding may sit between
them. An `A` byte is hexadecimal `41`; a null terminator is `00`.

</details>

## Step 2: Stack canary (8 minutes)

Predict the result, then change the stack-protector setting:

```bash
gcc -g -O0 -fstack-protector-all -fno-pie -no-pie -Wl,-z,noexecstack -o bo_canary bo_passcheck.c
./bo_canary
```

Type the **same input** from Step 1. Also try a longer run of `A`s if the first
input produces no diagnostic. Keep these two trials separate in your notes.

**Q4.** Did the action appear? Did you see `stack smashing detected`? Reopen
`bo_canary` in GDB using the commands above and compare the variable addresses.
Did the compiler move the flag? Why can “no login” and “canary detected damage”
be different observations?

The compiler checks the guard when the protected function exits. It may also
change the arrangement of local variables. A guard does not check each write
or undo actions performed before its check.

## Step 3: NX and ASLR (8 minutes)

### Step 3A: Observe NX on the baseline

After the NX introduction, reuse the baseline binary from Step 1. Its canary
is off and it already requests a non-executable stack:

```bash
readelf -W -l bo_passcheck | grep GNU_STACK
./bo_passcheck
```

`GNU_STACK` should show `RW`, without `E`. Predict what NX checks, then type
your same wrong input. Record whether the action appears. This observes the
scope of an existing protection; it does not compare NX-on against NX-off.

### Step 3B: Add PIE and check OS ASLR

After the ASLR introduction, keep the non-executable-stack setting and make
the executable position independent, with the canary still off:

```bash
gcc -g -O0 -fno-stack-protector -fPIE -pie -Wl,-z,noexecstack -o bo_nx bo_passcheck.c
readelf -W -l bo_nx | grep GNU_STACK
readelf -h bo_nx | grep Type
cat /proc/sys/kernel/randomize_va_space
./bo_nx
```

`GNU_STACK` should show `RW`, without `E`. `DYN` identifies the PIE executable
in this build. A kernel setting of `1` or `2` enables ASLR; `0` means it is off.
These are different checks: `-pie` does not switch on ASLR or NX by itself.

**Q5.** Compare your observations in Steps 3A and 3B. Does the same input
trigger the action in each? Does this input
contain machine code to execute, or an absolute address to jump to? Explain
what each defence checks and why that matters here.

To observe address changes, use GDB on `bo_nx`, keeping
`set disable-randomization off`. Stop at `main`, print `&main` and `&password`,
then repeat `run` and the two prints. GDB normally tries to disable ASLR;
that setting keeps the OS behaviour. Do not change your host's ASLR setting.

## Step 4: Repair the source (10 minutes)

Before opening `bo_fixed.c`, propose a replacement for `gets`. State the maximum
number of input bytes and what to do if the line is too long. Then compare your
proposal with the supplied repair:

```bash
cat bo_fixed.c
```

The core read is:

```c
if (fgets(password, sizeof password, stdin) == NULL) return 1;
```

With a capacity of 16, a successful `fgets` stores at most **15 input bytes plus
`\0`**. The full file checks whether the line ended, removes a stored newline,
and rejects a line over 15 bytes instead of silently using its prefix. Exactly
15 bytes followed by a newline or EOF are allowed. This is a one-shot program
for terminal text without embedded NUL bytes; a retry loop would also need to
discard the rest of an oversized line.

First test the repair **without** a canary, to isolate the source change:

```bash
gcc -g -O0 -Wall -Wextra -fno-stack-protector -fno-pie -no-pie -Wl,-z,noexecstack -o bo_fixed bo_fixed.c
./bo_fixed
```

Try your attack, `password1`, a short wrong password, and lengths 15 and 16.
Then run the boundary checks and add hardening:

```bash
python3 check-fixed.py ./bo_fixed
gcc -O2 -Wall -Wextra -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE -pie -Wl,-z,noexecstack -o bo_fixed_hardened bo_fixed.c
python3 check-fixed.py ./bo_fixed_hardened
```

**Q6.** Which result shows that the source repair works independently of the
canary? Why do we test valid inputs as well as the attack? What would go wrong
if a repair simply rejected every input?

## Debrief (4 minutes)

**Q7.** Complete your evidence sheet. Separate the invalid write, its effect on
a data flag, and any later crash. Which build removes the invalid write? Which
mechanisms address only particular consequences?

**Q8.** Suppose a different overflow changes a saved return address. Explain
how a canary, NX and ASLR could obstruct different parts of that attack. Would
NX block reuse of instructions in an already executable code page?

Submit or keep: your evidence sheet, your GDB observations, and brief answers
to Q1–Q8. A result different from the instructor's is useful when you explain
it using the measured layout.

## Troubleshooting and cleanup

- **Cannot connect to Docker:** start Docker Desktop, then retry on the host.
- **GDB: `ptrace: Function not implemented`:** check `uname -m` and remove any
  forced platform setting. Build natively; do not debug an emulated image.
- **GDB: `Operation not permitted`:** save your work, exit, and start a new lab
  container with `docker run --rm -it --cap-add=SYS_PTRACE bo-defences`. If local
  policy still blocks tracing, use the instructor's observations for Q2 and
  continue the interactive experiments. Do not use `--privileged`.
- **No bypass or different threshold:** use `-O0`, check the exact source and
  inspect the addresses. Separate locals have no guaranteed relative order.
- **Canary run says only `You fail!`:** inspect the changed layout and try a
  longer input. Absence of a diagnostic does not prove an in-bounds write.
- **Compiler warning about `gets`:** expected for the vulnerable source. A
  compile error is different; do not run an old binary after a failed build.

After saving your answers, `exit` leaves and removes the container. Optionally
run `docker rmi bo-defences` on the host to remove the image.
