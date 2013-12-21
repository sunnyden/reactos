/*
 * Copyright (C) 2008 Tony Wasserka
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include "d3dx9_36_private.h"

/* the combination of all possible D3DXSPRITE flags */
#define D3DXSPRITE_FLAGLIMIT 511

typedef struct _SPRITEVERTEX {
    D3DXVECTOR3 pos;
    DWORD col;
    D3DXVECTOR2 tex;
} SPRITEVERTEX;

typedef struct _SPRITE {
    IDirect3DTexture9 *texture;
    UINT texw, texh;
    RECT rect;
    D3DXVECTOR3 center;
    D3DXVECTOR3 pos;
    D3DCOLOR color;
    D3DXMATRIX transform;
} SPRITE;

typedef struct ID3DXSpriteImpl
{
    ID3DXSprite ID3DXSprite_iface;
    LONG ref;

    IDirect3DDevice9 *device;
    IDirect3DVertexDeclaration9 *vdecl;
    IDirect3DStateBlock9 *stateblock;
    D3DXMATRIX transform;
    D3DXMATRIX view;
    DWORD flags;
    BOOL ready;

    /* Store the relevant caps to prevent multiple GetDeviceCaps calls */
    DWORD texfilter_caps;
    DWORD maxanisotropy;
    DWORD alphacmp_caps;

    SPRITE *sprites;
    int sprite_count;      /* number of sprites to be drawn */
    int allocated_sprites; /* number of (pre-)allocated sprites */
} ID3DXSpriteImpl;

static inline ID3DXSpriteImpl *impl_from_ID3DXSprite(ID3DXSprite *iface)
{
    return CONTAINING_RECORD(iface, ID3DXSpriteImpl, ID3DXSprite_iface);
}

static HRESULT WINAPI ID3DXSpriteImpl_QueryInterface(ID3DXSprite *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXSprite)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXSpriteImpl_AddRef(ID3DXSprite *iface)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);
    ULONG ref=InterlockedIncrement(&This->ref);
    TRACE("(%p)->(): AddRef from %d\n", This, ref-1);
    return ref;
}

static ULONG WINAPI ID3DXSpriteImpl_Release(ID3DXSprite *iface)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);
    ULONG ref=InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): ReleaseRef to %d\n", This, ref);

    if(ref==0) {
        if(This->sprites) {
            int i;
            for(i=0;i<This->sprite_count;i++)
                if(This->sprites[i].texture)
                    IDirect3DTexture9_Release(This->sprites[i].texture);

            HeapFree(GetProcessHeap(), 0, This->sprites);
        }
        if(This->stateblock) IDirect3DStateBlock9_Release(This->stateblock);
        if(This->vdecl) IDirect3DVertexDeclaration9_Release(This->vdecl);
        if(This->device) IDirect3DDevice9_Release(This->device);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI ID3DXSpriteImpl_GetDevice(struct ID3DXSprite *iface, struct IDirect3DDevice9 **device)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);

    TRACE("(%p)->(%p)\n", This, device);

    if(device==NULL) return D3DERR_INVALIDCALL;
    *device=This->device;
    IDirect3DDevice9_AddRef(This->device);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXSpriteImpl_GetTransform(ID3DXSprite *iface, D3DXMATRIX *transform)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);

    TRACE("(%p)->(%p)\n", This, transform);

    if(transform==NULL) return D3DERR_INVALIDCALL;
    *transform=This->transform;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXSpriteImpl_SetTransform(ID3DXSprite *iface, CONST D3DXMATRIX *transform)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);

    TRACE("(%p)->(%p)\n", This, transform);

    if(transform==NULL) return D3DERR_INVALIDCALL;
    This->transform=*transform;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXSpriteImpl_SetWorldViewRH(ID3DXSprite *iface, CONST D3DXMATRIX *world,
        CONST D3DXMATRIX *view)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);
    FIXME("(%p)->(%p, %p): stub\n", This, world, view);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXSpriteImpl_SetWorldViewLH(ID3DXSprite *iface, CONST D3DXMATRIX *world,
        CONST D3DXMATRIX *view)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);
    FIXME("(%p)->(%p, %p): stub\n", This, world, view);
    return E_NOTIMPL;
}

/* Helper function */
static void set_states(ID3DXSpriteImpl *object)
{
    D3DXMATRIX mat;
    D3DVIEWPORT9 vp;

    /* Miscellaneous stuff */
    IDirect3DDevice9_SetVertexShader(object->device, NULL);
    IDirect3DDevice9_SetPixelShader(object->device, NULL);
    IDirect3DDevice9_SetNPatchMode(object->device, 0.0f);

    /* Render states */
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_ALPHABLENDENABLE, TRUE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_ALPHAFUNC, D3DCMP_GREATER);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_ALPHAREF, 0x00);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_ALPHATESTENABLE, object->alphacmp_caps);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_BLENDOP, D3DBLENDOP_ADD);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_CLIPPING, TRUE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_CLIPPLANEENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE |
                                    D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_CULLMODE, D3DCULL_NONE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_ENABLEADAPTIVETESSELLATION, FALSE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_FILLMODE, D3DFILL_SOLID);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_FOGENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_LIGHTING, FALSE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_RANGEFOGENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_SPECULARENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_SRGBWRITEENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_STENCILENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_VERTEXBLEND, FALSE);
    IDirect3DDevice9_SetRenderState(object->device, D3DRS_WRAP0, 0);

    /* Texture stage states */
    IDirect3DDevice9_SetTextureStageState(object->device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(object->device, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    IDirect3DDevice9_SetTextureStageState(object->device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(object->device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(object->device, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    IDirect3DDevice9_SetTextureStageState(object->device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(object->device, 0, D3DTSS_TEXCOORDINDEX, 0);
    IDirect3DDevice9_SetTextureStageState(object->device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    IDirect3DDevice9_SetTextureStageState(object->device, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    IDirect3DDevice9_SetTextureStageState(object->device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);

    /* Sampler states */
    IDirect3DDevice9_SetSamplerState(object->device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    IDirect3DDevice9_SetSamplerState(object->device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    if(object->texfilter_caps & D3DPTFILTERCAPS_MAGFANISOTROPIC)
        IDirect3DDevice9_SetSamplerState(object->device, 0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
    else IDirect3DDevice9_SetSamplerState(object->device, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    IDirect3DDevice9_SetSamplerState(object->device, 0, D3DSAMP_MAXMIPLEVEL, 0);
    IDirect3DDevice9_SetSamplerState(object->device, 0, D3DSAMP_MAXANISOTROPY, object->maxanisotropy);

    if(object->texfilter_caps & D3DPTFILTERCAPS_MINFANISOTROPIC)
        IDirect3DDevice9_SetSamplerState(object->device, 0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
    else IDirect3DDevice9_SetSamplerState(object->device, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

    if(object->texfilter_caps & D3DPTFILTERCAPS_MIPFLINEAR)
        IDirect3DDevice9_SetSamplerState(object->device, 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    else IDirect3DDevice9_SetSamplerState(object->device, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

    IDirect3DDevice9_SetSamplerState(object->device, 0, D3DSAMP_MIPMAPLODBIAS, 0);
    IDirect3DDevice9_SetSamplerState(object->device, 0, D3DSAMP_SRGBTEXTURE, 0);

    /* Matrices */
    D3DXMatrixIdentity(&mat);
    IDirect3DDevice9_SetTransform(object->device, D3DTS_WORLD, &mat);
    IDirect3DDevice9_SetTransform(object->device, D3DTS_VIEW, &object->view);
    IDirect3DDevice9_GetViewport(object->device, &vp);
    D3DXMatrixOrthoOffCenterLH(&mat, vp.X+0.5f, (float)vp.Width+vp.X+0.5f, (float)vp.Height+vp.Y+0.5f, vp.Y+0.5f, vp.MinZ, vp.MaxZ);
    IDirect3DDevice9_SetTransform(object->device, D3DTS_PROJECTION, &mat);
}

static HRESULT WINAPI ID3DXSpriteImpl_Begin(ID3DXSprite *iface, DWORD flags)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);
    HRESULT hr;

    TRACE("(%p)->(%#x)\n", This, flags);

    if(flags>D3DXSPRITE_FLAGLIMIT || This->ready) return D3DERR_INVALIDCALL;

/* TODO: Implement flags:
D3DXSPRITE_BILLBOARD: makes the sprite always face the camera
D3DXSPRITE_DONOTMODIFY_RENDERSTATE: name says it all
D3DXSPRITE_OBJECTSPACE: do not change device transforms
D3DXSPRITE_SORT_DEPTH_BACKTOFRONT: sort by position
D3DXSPRITE_SORT_DEPTH_FRONTTOBACK: sort by position
D3DXSPRITE_SORT_TEXTURE: sort by texture (so that it doesn't change too often)
*/
/* Seems like alpha blending is always enabled, regardless of D3DXSPRITE_ALPHABLEND flag */
    if(flags & (D3DXSPRITE_BILLBOARD |
                D3DXSPRITE_DONOTMODIFY_RENDERSTATE | D3DXSPRITE_OBJECTSPACE |
                D3DXSPRITE_SORT_DEPTH_BACKTOFRONT))
        FIXME("Flags unsupported: %#x\n", flags);
    /* These flags should only matter to performances */
    else if(flags & (D3DXSPRITE_SORT_DEPTH_FRONTTOBACK | D3DXSPRITE_SORT_TEXTURE))
        TRACE("Flags unsupported: %#x\n", flags);

    if(This->vdecl==NULL) {
        static const D3DVERTEXELEMENT9 elements[] =
        {
            { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
            { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
            { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
            D3DDECL_END()
        };
        IDirect3DDevice9_CreateVertexDeclaration(This->device, elements, &This->vdecl);
    }

    if(!(flags & D3DXSPRITE_DONOTSAVESTATE)) {
        if(This->stateblock==NULL) {
            /* Tell our state block what it must store */
            hr=IDirect3DDevice9_BeginStateBlock(This->device);
            if(hr!=D3D_OK) return hr;

            set_states(This);

            IDirect3DDevice9_SetVertexDeclaration(This->device, This->vdecl);
            IDirect3DDevice9_SetStreamSource(This->device, 0, NULL, 0, sizeof(SPRITEVERTEX));
            IDirect3DDevice9_SetIndices(This->device, NULL);
            IDirect3DDevice9_SetTexture(This->device, 0, NULL);

            IDirect3DDevice9_EndStateBlock(This->device, &This->stateblock);
        }
        IDirect3DStateBlock9_Capture(This->stateblock); /* Save current state */
    }

    /* Apply device state */
    set_states(This);

    This->flags=flags;
    This->ready=TRUE;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXSpriteImpl_Draw(ID3DXSprite *iface, IDirect3DTexture9 *texture,
        const RECT *rect, const D3DXVECTOR3 *center, const D3DXVECTOR3 *position, D3DCOLOR color)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);
    D3DSURFACE_DESC texdesc;

    TRACE("iface %p, texture %p, rect %s, center %p, position %p, color 0x%08x.\n",
            iface, texture, wine_dbgstr_rect(rect), center, position, color);

    if(texture==NULL) return D3DERR_INVALIDCALL;
    if(!This->ready) return D3DERR_INVALIDCALL;

    if(This->allocated_sprites==0) {
        This->sprites=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 32*sizeof(SPRITE));
        This->allocated_sprites=32;
    } else if(This->allocated_sprites<=This->sprite_count) {
        This->allocated_sprites=This->allocated_sprites*3/2;
        This->sprites=HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->sprites, This->allocated_sprites*sizeof(SPRITE));
    }
    This->sprites[This->sprite_count].texture=texture;
    if(!(This->flags & D3DXSPRITE_DO_NOT_ADDREF_TEXTURE))
        IDirect3DTexture9_AddRef(texture);

    /* Reuse the texture desc if possible */
    if(This->sprite_count) {
        if(This->sprites[This->sprite_count-1].texture!=texture) {
            IDirect3DTexture9_GetLevelDesc(texture, 0, &texdesc);
        } else {
            texdesc.Width=This->sprites[This->sprite_count-1].texw;
            texdesc.Height=This->sprites[This->sprite_count-1].texh;
        }
    } else IDirect3DTexture9_GetLevelDesc(texture, 0, &texdesc);

    This->sprites[This->sprite_count].texw=texdesc.Width;
    This->sprites[This->sprite_count].texh=texdesc.Height;

    if(rect==NULL) {
        This->sprites[This->sprite_count].rect.left=0;
        This->sprites[This->sprite_count].rect.top=0;
        This->sprites[This->sprite_count].rect.right=texdesc.Width;
        This->sprites[This->sprite_count].rect.bottom=texdesc.Height;
    } else This->sprites[This->sprite_count].rect=*rect;

    if(center==NULL) {
        This->sprites[This->sprite_count].center.x=0.0f;
        This->sprites[This->sprite_count].center.y=0.0f;
        This->sprites[This->sprite_count].center.z=0.0f;
    } else This->sprites[This->sprite_count].center=*center;

    if(position==NULL) {
        This->sprites[This->sprite_count].pos.x=0.0f;
        This->sprites[This->sprite_count].pos.y=0.0f;
        This->sprites[This->sprite_count].pos.z=0.0f;
    } else This->sprites[This->sprite_count].pos=*position;

    This->sprites[This->sprite_count].color=color;
    This->sprites[This->sprite_count].transform=This->transform;
    This->sprite_count++;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXSpriteImpl_Flush(ID3DXSprite *iface)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);
    SPRITEVERTEX *vertices;
    int i, count=0, start;

    TRACE("(%p)->()\n", This);

    if(!This->ready) return D3DERR_INVALIDCALL;
    if(!This->sprite_count) return D3D_OK;

/* TODO: use of a vertex buffer here */
    vertices=HeapAlloc(GetProcessHeap(), 0, sizeof(SPRITEVERTEX)*6*This->sprite_count);

    for(start=0;start<This->sprite_count;start+=count,count=0) {
        i=start;
        while(i<This->sprite_count &&
              (count==0 || This->sprites[i].texture==This->sprites[i-1].texture)) {
            float spritewidth=(float)This->sprites[i].rect.right-(float)This->sprites[i].rect.left;
            float spriteheight=(float)This->sprites[i].rect.bottom-(float)This->sprites[i].rect.top;

            vertices[6*i  ].pos.x = This->sprites[i].pos.x - This->sprites[i].center.x;
            vertices[6*i  ].pos.y = This->sprites[i].pos.y - This->sprites[i].center.y;
            vertices[6*i  ].pos.z = This->sprites[i].pos.z - This->sprites[i].center.z;
            vertices[6*i+1].pos.x = spritewidth + This->sprites[i].pos.x - This->sprites[i].center.x;
            vertices[6*i+1].pos.y = This->sprites[i].pos.y - This->sprites[i].center.y;
            vertices[6*i+1].pos.z = This->sprites[i].pos.z - This->sprites[i].center.z;
            vertices[6*i+2].pos.x = spritewidth + This->sprites[i].pos.x - This->sprites[i].center.x;
            vertices[6*i+2].pos.y = spriteheight + This->sprites[i].pos.y - This->sprites[i].center.y;
            vertices[6*i+2].pos.z = This->sprites[i].pos.z - This->sprites[i].center.z;
            vertices[6*i+3].pos.x = This->sprites[i].pos.x - This->sprites[i].center.x;
            vertices[6*i+3].pos.y = spriteheight + This->sprites[i].pos.y - This->sprites[i].center.y;
            vertices[6*i+3].pos.z = This->sprites[i].pos.z - This->sprites[i].center.z;
            vertices[6*i  ].col   = This->sprites[i].color;
            vertices[6*i+1].col   = This->sprites[i].color;
            vertices[6*i+2].col   = This->sprites[i].color;
            vertices[6*i+3].col   = This->sprites[i].color;
            vertices[6*i  ].tex.x = (float)This->sprites[i].rect.left / (float)This->sprites[i].texw;
            vertices[6*i  ].tex.y = (float)This->sprites[i].rect.top / (float)This->sprites[i].texh;
            vertices[6*i+1].tex.x = (float)This->sprites[i].rect.right / (float)This->sprites[i].texw;
            vertices[6*i+1].tex.y = (float)This->sprites[i].rect.top / (float)This->sprites[i].texh;
            vertices[6*i+2].tex.x = (float)This->sprites[i].rect.right / (float)This->sprites[i].texw;
            vertices[6*i+2].tex.y = (float)This->sprites[i].rect.bottom / (float)This->sprites[i].texh;
            vertices[6*i+3].tex.x = (float)This->sprites[i].rect.left / (float)This->sprites[i].texw;
            vertices[6*i+3].tex.y = (float)This->sprites[i].rect.bottom / (float)This->sprites[i].texh;

            vertices[6*i+4]=vertices[6*i];
            vertices[6*i+5]=vertices[6*i+2];

            D3DXVec3TransformCoordArray(&vertices[6*i].pos, sizeof(SPRITEVERTEX),
                                        &vertices[6*i].pos, sizeof(SPRITEVERTEX),
                                        &This->sprites[i].transform, 6);
            count++;
            i++;
        }

        IDirect3DDevice9_SetTexture(This->device, 0, (struct IDirect3DBaseTexture9 *)This->sprites[start].texture);
        IDirect3DDevice9_SetVertexDeclaration(This->device, This->vdecl);

        IDirect3DDevice9_DrawPrimitiveUP(This->device, D3DPT_TRIANGLELIST, 2*count, vertices+6*start, sizeof(SPRITEVERTEX));
    }
    HeapFree(GetProcessHeap(), 0, vertices);

    if(!(This->flags & D3DXSPRITE_DO_NOT_ADDREF_TEXTURE))
        for(i=0;i<This->sprite_count;i++)
            IDirect3DTexture9_Release(This->sprites[i].texture);

    This->sprite_count=0;

    /* Flush may be called more than once, so we don't reset This->ready here */

    return D3D_OK;
}

static HRESULT WINAPI ID3DXSpriteImpl_End(ID3DXSprite *iface)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);

    TRACE("(%p)->()\n", This);

    if(!This->ready) return D3DERR_INVALIDCALL;

    ID3DXSprite_Flush(iface);

    if(!(This->flags & D3DXSPRITE_DONOTSAVESTATE))
        if(This->stateblock) IDirect3DStateBlock9_Apply(This->stateblock); /* Restore old state */

    This->ready=FALSE;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXSpriteImpl_OnLostDevice(ID3DXSprite *iface)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);

    TRACE("(%p)->()\n", This);

    if(This->stateblock) IDirect3DStateBlock9_Release(This->stateblock);
    if(This->vdecl) IDirect3DVertexDeclaration9_Release(This->vdecl);
    This->vdecl=NULL;
    This->stateblock=NULL;

    /* Reset some variables */
    ID3DXSprite_OnResetDevice(iface);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXSpriteImpl_OnResetDevice(ID3DXSprite *iface)
{
    ID3DXSpriteImpl *This = impl_from_ID3DXSprite(iface);
    int i;

    TRACE("(%p)->()\n", This);

    for(i=0;i<This->sprite_count;i++)
        if(This->sprites[i].texture)
            IDirect3DTexture9_Release(This->sprites[i].texture);

    This->sprite_count=0;

    This->flags=0;
    This->ready=FALSE;

    /* keep matrices */
    /* device objects get restored on Begin */

    return D3D_OK;
}

static const ID3DXSpriteVtbl D3DXSprite_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXSpriteImpl_QueryInterface,
    ID3DXSpriteImpl_AddRef,
    ID3DXSpriteImpl_Release,
    /*** ID3DXSprite methods ***/
    ID3DXSpriteImpl_GetDevice,
    ID3DXSpriteImpl_GetTransform,
    ID3DXSpriteImpl_SetTransform,
    ID3DXSpriteImpl_SetWorldViewRH,
    ID3DXSpriteImpl_SetWorldViewLH,
    ID3DXSpriteImpl_Begin,
    ID3DXSpriteImpl_Draw,
    ID3DXSpriteImpl_Flush,
    ID3DXSpriteImpl_End,
    ID3DXSpriteImpl_OnLostDevice,
    ID3DXSpriteImpl_OnResetDevice
};

HRESULT WINAPI D3DXCreateSprite(struct IDirect3DDevice9 *device, struct ID3DXSprite **sprite)
{
    ID3DXSpriteImpl *object;
    D3DCAPS9 caps;

    TRACE("(%p, %p)\n", device, sprite);

    if(device==NULL || sprite==NULL) return D3DERR_INVALIDCALL;

    object=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3DXSpriteImpl));
    if(object==NULL) {
        *sprite=NULL;
        return E_OUTOFMEMORY;
    }
    object->ID3DXSprite_iface.lpVtbl = &D3DXSprite_Vtbl;
    object->ref=1;
    object->device=device;
    IUnknown_AddRef(device);

    object->vdecl=NULL;
    object->stateblock=NULL;

    D3DXMatrixIdentity(&object->transform);
    D3DXMatrixIdentity(&object->view);

    IDirect3DDevice9_GetDeviceCaps(device, &caps);
    object->texfilter_caps=caps.TextureFilterCaps;
    object->maxanisotropy=caps.MaxAnisotropy;
    object->alphacmp_caps=caps.AlphaCmpCaps;

    ID3DXSprite_OnResetDevice(&object->ID3DXSprite_iface);

    object->sprites=NULL;
    object->allocated_sprites=0;
    *sprite=&object->ID3DXSprite_iface;

    return D3D_OK;
}
