#pragma once

ULONG
INTERNAL_CALL
XFORMOBJ_iSetXform(
	OUT XFORMOBJ *pxo,
	IN XFORML * pxform);

ULONG
INTERNAL_CALL
XFORMOBJ_iCombine(
	IN XFORMOBJ *pxo,
	IN XFORMOBJ *pxo1,
	IN XFORMOBJ *pxo2);

ULONG
INTERNAL_CALL
XFORMOBJ_iCombineXform(
	IN XFORMOBJ *pxo,
	IN XFORMOBJ *pxo1,
	IN XFORML *pxform,
	IN BOOL bLeftMultiply);

ULONG
INTERNAL_CALL
XFORMOBJ_Inverse(
	OUT XFORMOBJ *pxoDst,
	IN XFORMOBJ *pxoSrc);
