<module name="hid" type="win32dll" baseaddress="${BASEADDRESS_HID}" installbase="system32" installname="hid.dll" unicode="yes">
	<importlibrary definition="hid.spec" />
	<include base="hid">.</include>
	<library>ntdll</library>
	<file>hid.c</file>
	<file>stubs.c</file>
	<file>hid.rc</file>
	<pch>precomp.h</pch>
</module>
