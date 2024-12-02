# FreeRTOS_Project_558
The source code was based on the CORTEX_MPS2_QEMU_IAR_GCC example from [Running with VSCode Launch Configurations](https://github.com/FreeRTOS/FreeRTOS/tree/main/FreeRTOS/Demo/CORTEX_MPS2_QEMU_IAR_GCC)

## Prerequisites
* Install [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) in VSCode.
* Install [Native Debug](https://marketplace.visualstudio.com/items?itemName=webfreak.debug)
* Install [arm-none-eabi-gcc](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads).
* Install [qemu-system-arm](https://www.qemu.org/download/)
* Install GNU make utility.
* Ensure the required binaries are in PATH with ```arm-none-eabi-gcc --version```, ```arm-none-eabi-gdb --version```, and ```make --version```.
* Download the [FreeRTOS 202406.01 LTS](https://www.freertos.org/Documentation/02-Kernel/01-About-the-FreeRTOS-kernel/03-Download-freeRTOS/01-DownloadFreeRTOS)
    * Ensure in the kernal you have ```portable/MemMang```
* Ensure qemu is installed with ```qemu-system-arm --version```

## Building and Running
1. Open VSCode to the folder ```CORTEX_MPS2_QEMU_IAR_GCC```.
2. Open ```.vscode/launch.json```, and ensure the ```miDebuggerPath``` variable is set to the path where arm-none-eabi-gdb is on your machine.
3. Open ```main.c``` and update ```MAX_TICK_COUNT``` to impact how many ticks before the program stops
4. Open ```main_rms_deferred.c``` and update the following settings depending on requirements:
- SIMPLE_LOW_DELAY: delay in milliseconds for the low task (default 100)
- SIMPLE_MEDIUM_DELAY: delay in milliseconds for the medium task (default 200)
- SIMPLE_HIGH_DELAY: delay in milliseconds for the high task (delay 300)
- SIMPLE_DEFERRER_SERVER_DELAY: delay in milliseconds for the deferred server (default 10)
- SIMPLE_LOW_COMPUTATION: computation rate in ticks for the low task (default 2)
- SIMPLE_MEDIUM_COMPUTATION: computation rate in ticks for the medium task (default 3)
- SIMPLE_HIGH_COMPUTATION: computation rate in ticks for the high task (default 5)
- SIMPLE_APERIODIC_COMPUTATION_MIN: minimum range of computation rate in ticks for the aperiodic tasks (default 1) \[inclusive\]
- SIMPLE_APERIODIC_COMPUTATION_MAX: maximum range of computation rate in ticks for the aperiodic tasks (default 7) \[inclusive\]
- APERIODIC_DELAY_MIN: maximum range of delay in milliseconds for the aperiodic tasks (default 30) \[inclusive\]
- APERIODIC_DELAY_MAX: minimum range of delay in milliseconds for the aperiodic tasks (default 100) \[inclusive\]
- SERVER_BUDGET_MS: deferred server budget in millliseconds (default 50)
- SERVER_PERIOD_MS: deferred server period in milliseconds (default 100)
5. Update the ```buld/gcc/Makefile``` and ensure the paths are accurate.
6. On the VSCode left side panel, select the “Run and Debug” button. Then select “Launch QEMU RTOSDemo” from the dropdown on the top right and press the play button. This will build, run, and attach a debugger to the demo program.
- You'll need to progress past the build/gcc/startup_gcc.c ```main()``` method and the main.c ```prvUARTInit()``` method to get the program to execute
