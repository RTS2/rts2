#include "rts2scriptblock.h"

Rts2ScriptElementBlock::Rts2ScriptElementBlock (Rts2Script * in_script):Rts2ScriptElement
  (in_script)
{

}

Rts2ScriptElementBlock::~Rts2ScriptElementBlock (void)
{

}

void
Rts2ScriptElementBlock::postEvent (Rts2Event * event)
{

}

int
Rts2ScriptElementBlock::defnextCommand (Rts2DevClient * client,
					Rts2Command ** new_command,
					char new_device[DEVICE_NAME_SIZE])
{

}

int
Rts2ScriptElementBlock::nextCommand (Rts2DevClientCamera * camera,
				     Rts2Command ** new_command,
				     char new_device[DEVICE_NAME_SIZE])
{

}

int
Rts2ScriptElementBlock::nextCommand (Rts2DevClientPhot * phot,
				     Rts2Command ** new_command,
				     char new_device[DEVICE_NAME_SIZE])
{

}

int
Rts2ScriptElementBlock::processImage (Rts2Image * image)
{

}

int
Rts2ScriptElementBlock::waitForSignal (int in_sig)
{

}

void
Rts2ScriptElementBlock::cancelCommands ()
{

}
