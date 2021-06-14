# Overview

Low Level MicroEJ Core Engine API implementation over Micrium OS.

This component implements the `LLMJVM` Low Level API for MicroEJ platforms connected to a Board Support Package based on [Micrium OS](https://www.silabs.com/developers/micrium-os).

See the MicroEJ documentation for a description of the `LLMJVM` functions:
- [LLMJVM: MicroEJ Core Engine](https://docs.microej.com/en/latest/PlatformDeveloperGuide/appendix/llapi.html#llmjvm-microej-core-engine)
- [MicroEJ Core Engine: Implementation](https://docs.microej.com/en/latest/PlatformDeveloperGuide/coreEngine.html#implementation)

# Usage

1. Install ``src`` and ``inc`` directories in your Board Support Package. They can be automatically downloaded using the following command lines:
    ```sh
    svn export --force https://github.com/MicroEJ/AbstractionLayer-Core-MicriumOS/trunk/inc [path_to_bsp_directory]    
    svn export --force https://github.com/MicroEJ/AbstractionLayer-Core-MicriumOS/trunk/src [path_to_bsp_directory]
    ```
2. Implement the MicroEJ time functions, as described in [microej_time.h](./inc/microej_time.h).

3. The `LLMJVM_IMPL_scheduleRequest` schedule request function in [LLMJVM_MicriumOS.c] uses a software timer. In order to correctly schedule MicroEJ threads, check the following elements in the MicriumOS configuration file:
 - `OS_CFG_TICK_EN`: needs to be enabled to get access to all time-based services in Micrium OS Kernel. The `OS_ConfigureTickTask` function can be used to configure the tick task stack size and priority.
  The `OS_ConfigureTickRate` function can be used to set the OS tick rate. The MicroEJ task should have a lower priority than the tick task.
 - `OS_CFG_TMR_EN`: needs to be enabled to get access to all software-based timers in Micrium OS Kernel. The `OS_ConfigureTmrTask` function can be used to configure the timer task stack size and priority. 
  The `OSTmrCreate` function can be used to create a timer with the desired periodicity.  The MicroEJ task should have a lower priority than the timer task.

# Requirements

None.

# Validation

This Abstraction Layer implementation can be validated in the target Board Support Package using the [MicroEJ Core Validation](https://github.com/MicroEJ/PlatformQualificationTools/tree/master/tests/core/java/microej-core-validation) Platform Qualification Tools project.

Here is a non exhaustive list of tested environments:
- Hardware
  - Silicon Labs EFR32BG22
- Compilers / development environments:
  - IAR Embedded Workbench 8.50.6
- Micrium OS versions:
  - 5.9.0

## MISRA Compliance

The implementation is MISRA-compliant (MISRA C 2004) with the following observed deviations:
| Deviation | Category |                                                 Justification                                                 |
|:---------:|:--------:|:-------------------------------------------------------------------------------------------------------------:|
| Rule 11.3 | Advisory | A deviation from this rule is necessary as the OSTCBCurPtr has to be cast to an int32_t to get a task id      | 
| Rule 16.4 | Required | A deviation from this rule is necessary as long as the LLMJVM_IMPL_getCurrentTime declaration does not match  | 


# Dependencies

- MicroEJ Architecture ``7.x`` or higher.
- Micrium OS ``5.9.0`` or higher.

# Source

N/A.

# Restrictions

None.

---
_Copyright 2021 MicroEJ Corp. All rights reserved._  
_This library is provided in source code for use, modification and test, subject to license terms._    
_Any modification of the source code will break MicroEJ Corp. warranties on the whole library._ 
