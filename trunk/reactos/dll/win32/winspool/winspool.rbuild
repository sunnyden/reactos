<module name="winspool" type="win32dll" extension=".drv" baseaddress="${BASEADDRESS_WINSPOOL}" installbase="system32" installname="winspool.drv" allowwarnings="true" unicode="yes">
	<importlibrary definition="winspool.def" />
	<include base="winspool">.</include>
	<define name="_DISABLE_TIDENTS" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<file>info.c</file>
	<file>stubs.c</file>
	<file>winspool.rc</file>
</module>
