<module name="snmpapi" type="win32dll" baseaddress="${BASEADDRESS_SNMPAPI}" installbase="system32" installname="snmpapi.dll" unicode="yes">
	<importlibrary definition="snmpapi.spec.def" />
	<include base="snmpapi">.</include>
	<define name="_DISABLE_TIDENTS" />
	<library>ntdll</library>
	<library>kernel32</library>
	<file>snmpapi.c</file>
	<file>snmpapi.rc</file>
	<file>snmpapi.spec</file>
</module>
