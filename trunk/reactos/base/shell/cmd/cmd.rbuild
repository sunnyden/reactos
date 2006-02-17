<module name="cmd_base" type="objectlibrary">
	<include base="ReactOS">include/wine</include>
	<include base="cmd_base">.</include>
	<define name="__USE_W32API" />
	<define name="ANONYMOUSUNIONS" />
	<define name="_WIN32_WINNT">0x0501</define>
	<pch>precomp.h</pch>
	<compilationunit name="unit.c">
		<file>alias.c</file>
		<file>attrib.c</file>
		<file>batch.c</file>
		<file>beep.c</file>
		<file>call.c</file>
		<file>chcp.c</file>
		<file>choice.c</file>
		<file>cls.c</file>
		<file>cmd.c</file>
		<file>cmdinput.c</file>
		<file>cmdtable.c</file>
		<file>color.c</file>
		<file>console.c</file>
		<file>copy.c</file>
		<file>date.c</file>
		<file>del.c</file>
		<file>delay.c</file>
		<file>dir.c</file>
		<file>dirstack.c</file>
		<file>echo.c</file>
		<file>error.c</file>
		<file>filecomp.c</file>
		<file>for.c</file>
		<file>free.c</file>
		<file>goto.c</file>
		<file>history.c</file>
		<file>if.c</file>
		<file>internal.c</file>
		<file>label.c</file>
		<file>locale.c</file>
		<file>memory.c</file>
		<file>misc.c</file>
		<file>move.c</file>
		<file>msgbox.c</file>
		<file>path.c</file>
		<file>pause.c</file>
		<file>prompt.c</file>
		<file>redir.c</file>
		<file>ren.c</file>
		<file>screen.c</file>
		<file>set.c</file>
		<file>shift.c</file>
		<file>start.c</file>
		<file>strtoclr.c</file>
		<file>time.c</file>
		<file>timer.c</file>
		<file>title.c</file>
		<file>type.c</file>
		<file>ver.c</file>
		<file>verify.c</file>
		<file>vol.c</file>
		<file>where.c</file>
		<file>window.c</file>
	</compilationunit>
</module>
<module name="cmd" type="win32cui" installbase="system32" installname="cmd.exe" usewrc="false">
	<include base="ReactOS">include/wine</include>
	<include base="cmd">.</include>
	<define name="__USE_W32API" />
	<define name="ANONYMOUSUNIONS" />
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>cmd_base</library>
	<file>main.c</file>
	<file>cmd.rc</file>
</module>
<directory name="tests">
	<xi:include href="tests/cmd_test.rbuild" />
</directory>
