/*
 * C
 *
 *  Copyright 2021 MicroEJ Corp. All rights reserved.
 *  This library is provided in source code for use, modification and test, subject to license terms.
 *  Any modification of the source code will break MicroEJ Corp. warranties on the whole library.
 *
 */

#ifndef SNI_H_
#define SNI_H_

#include <stdint.h>

/*
 * Creates and initializes a virtual machine.
 * This function MUST be called once before a call to <code>SNI_startVM()</code>.
 *
 * @return <code>null</code> if an error occurred, otherwise returns a virtual machine
 * instance.
 */
void* SNI_createVM(void);

/**
 * Starts the specified virtual machine and calls the <code>main()</code> method of the
 * Java application with the given String arguments.
 * This function returns when the Java application ends.
 *
 * The Java application ends when there are no more Java threads to run or when the Java
 * method <code>System.exit(int)</code> is called.
 *
 * @param vm a pointer returned by the <code>SNI_createVM()</code> function.
 *
 * @param argc number of string arguments in <code>argv</code>.
 *
 * @param argv array of string arguments given to the Java <code>main()</code> method.
 * May be null.
 *
 * @return 0 when the virtual machine ends normally or a negative value when an error
 * occurred.
 */
int32_t SNI_startVM(void* vm, int32_t argc, char** argv);

/*
 * Call this function after virtual machine execution to get the Java application exit
 * code.
 *
 * @param vm a pointer returned by the <code>SNI_createVM()</code> function.
 *
 * @return the value given to the <code>System.exit(exitCode)</code> or 0 if the Java
 * application ended without calling <code>System.exit(exitCode)</code>.
 */
int32_t SNI_getExitCode(void* vm);

/**
 * Releases all the virtual machine resources. This function must be called after the end
 * of the execution of the virtual machine. The vm pointer is no longer valid after this call.
 *
 * @param vm a pointer returned by the <code>SNI_createVM()</code> function.
 */
void SNI_destroyVM(void* vm);

#endif /* SNI_H_ */
