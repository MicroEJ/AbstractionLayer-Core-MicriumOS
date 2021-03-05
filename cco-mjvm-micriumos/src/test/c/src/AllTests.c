/*
 * C
 *
 * Copyright 2021 MicroEJ Corp. All rights reserved.
 * This library is provided in source code for use, modification and test, subject to license terms.
 * Any modification of the source code will break MicroEJ Corp. warranties on the whole library.
 */
#include <embUnit/embUnit.h>
#include "Outputter.h"
#include "TextUIRunner.h"
#include "XMLOutputter.h"

int main (int argc, const char* argv[])
{
	TextUIRunner_setOutputter(XMLOutputter_outputter());
	TextUIRunner_start();
	// Please add your tests here
	TextUIRunner_end();
	return 0;
}
