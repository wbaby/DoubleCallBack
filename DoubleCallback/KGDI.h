#include <wingdi.h>

//render
class Render
{
private:
    //obj
    HDC ScreenDC;
    HFONT CurFont;
    HBITMAP ScreenBitmap;
    HBITMAP ScaleScreenBitmap;

    //desc
    int ScrScale;
    int CurrentWidth;
    int CurrentHeight;

    //um buffer
    PVOID UserBuffer;
    PVOID DcAttrObject;
    PBYTE MappedTexture;
    PBYTE ScaleMappedTexture;
    
    //shared internals
    PVOID GetWin32k(const char* Name)
    {
        //win32kfull
        auto win32kFull = GetKernelModuleBase(E("win32kfull.sys"));
        auto func = GetProcAdress(win32kFull, Name);

        //win32kbase
        if (!func) {
            auto win32kBase = GetKernelModuleBase(E("win32kbase.sys"));
            func = GetProcAdress(win32kBase, Name);
        }

        //win32k
        if (!func) {
            auto win32k = GetKernelModuleBase(E("win32k.sys"));
            func = GetProcAdress(win32k, Name);
        }

        return func;
    }

    HPEN SelectPenInternal(HPEN hPen)
    {
        static PVOID NtGdiSelectPen_Fn = nullptr;
        if (!NtGdiSelectPen_Fn) {
            NtGdiSelectPen_Fn = EPtr(GetWin32k(E("NtGdiSelectPen")));
        }

        //call NtGdiSelectPen
        return CallPtr<HPEN>(EPtr(NtGdiSelectPen_Fn), EPtr(ScreenDC), hPen);
    }

    void RemoveObjInternal(HGDIOBJ Obj)
    {
        if (Obj)
        {
            static PVOID DeleteObject_Fn = nullptr;
            if (!DeleteObject_Fn) {
                DeleteObject_Fn = EPtr(GetWin32k(E("NtGdiDeleteObjectApp")));
            }

            //call NtGdiDeleteObjectApp
            CallPtr(EPtr(DeleteObject_Fn), Obj);
        }
    }

    HFONT SelectFontInternal(HFONT hFont)
    {
        static PVOID NtGdiSelectFont_Fn = nullptr;
        if (!NtGdiSelectFont_Fn) {
            NtGdiSelectFont_Fn = EPtr(GetWin32k(E("NtGdiSelectFont")));
        }

        //call NtGdiSelectFont
        return CallPtr<HFONT>(EPtr(NtGdiSelectFont_Fn), EPtr(ScreenDC), hFont);
    }

    HBITMAP SelectBitMapInternal(HBITMAP BitMap)
    {
        static PVOID NtGdiSelectBitmap_Fn = nullptr;
        if (!NtGdiSelectBitmap_Fn) {
            NtGdiSelectBitmap_Fn = EPtr(GetWin32k(E("NtGdiSelectBitmap")));
        }

        //call NtGdiSelectBitmap
        return CallPtr<HBITMAP>(EPtr(NtGdiSelectBitmap_Fn), EPtr(ScreenDC), BitMap);
    }

    _FI HGDIOBJ GetStockObjectInternal(ULONG Index)
    {
        //get GdiSharedHandleTable
        auto PEB = ImpCall(PsGetProcessPeb, ImpCall(IoGetCurrentProcess));
        auto GdiSharedHandleTable = *(ULONG64*)((ULONG64)PEB + 0xF8/*GdiSharedHandleTable*/);

        //get object
        const auto ObjArray = (ULONG64*)(GdiSharedHandleTable + 0x1800B0/*in GetStockObject*/);
        return (HGDIOBJ)ObjArray[Index];
    }

    _FI PVOID GetDCObjectInternal()
    {
        //get GdiSharedHandleTable
        auto PEB = ImpCall(PsGetProcessPeb, ImpCall(IoGetCurrentProcess));
        auto pGdiSharedHandleTable = *(ULONG64*)((ULONG64)PEB + 0xF8/*GdiSharedHandleTable*/);

        //get index
        #pragma warning(disable: 4311) 
        auto a1 = EPtr(ScreenDC);
        auto v2 = (unsigned __int16)a1 | ((unsigned int)a1 >> 8) & 0xFF0000; //shit rcast not works
        #pragma warning(default:4311)
        if ((unsigned int)v2 < 0x10000 || *(unsigned __int8*)(pGdiSharedHandleTable + 24i64 * (unsigned __int16)v2 + 13) == (unsigned int)v2 >> 16) {
            v2 = (unsigned __int16)v2;
        }

        //get object
        struct GDICELL { PVOID s1; PVOID s2; PVOID UserAddress; };
        return ((GDICELL*)pGdiSharedHandleTable)[v2].UserAddress;
    }

    //draw polygon & polyline
    void PolyLineInternal(POINT* Dots, ULONG64 NumDots, int Thick, COLORREF Color)
    {
        //decrt DC
        auto hDC = EPtr(ScreenDC);

        //NtGdiCreatePen
        static PVOID NtGdiCreatePen_Fn = nullptr;
        if (!NtGdiCreatePen_Fn) {
            NtGdiCreatePen_Fn = EPtr(GetWin32k(E("NtGdiCreatePen")));
        }

        //create & select pen
        auto Pen = CallPtr<HPEN>(EPtr(NtGdiCreatePen_Fn), PS_SOLID, Thick, Color, 0ull);
        auto oldPen = SelectPenInternal(Pen);

        //fill line
        static PVOID GrePolyPolyline = nullptr;
        if (!GrePolyPolyline)
            GrePolyPolyline = EPtr(GetWin32k(E("GrePolyPolyline")));
        CallPtr(EPtr(GrePolyPolyline), hDC, Dots, &NumDots, 1ull, NumDots);

        //cleanup
        SelectPenInternal(oldPen);
        RemoveObjInternal(Pen);
    }

    void PolygonInternal(POINT* Dots, ULONG64 NumDots, COLORREF Color)
    {
        //decrt
        auto hDC = EPtr(ScreenDC);
        auto Obj = EPtr(DcAttrObject);

        //select colors
        SelectPenInternal((HPEN)GetStockObjectInternal(NULL_PEN));
        *(DWORD*)((ULONG64)Obj + 0xC0) = Color; //in SetDCBrushColor
        *(DWORD*)((ULONG64)Obj + 0x98) |= 1u;   //in SetDCBrushColor

        //fill polygon
        static PVOID GrePolyPolygon = nullptr;
        if (!GrePolyPolygon)
            GrePolyPolygon = EPtr(GetWin32k(E("GrePolyPolygon")));
        CallPtr(EPtr(GrePolyPolygon), hDC, Dots, &NumDots, 1ull, NumDots);
    }

    //fast math
    _FI float CosAdd(float x) {
        float x2 = x * x;
        const float c1 = 0.99940307f;
        const float c2 = -0.49558072f;
        const float c3 = 0.03679168f;
        return (c1 + x2 * (c2 + c3 * x2));
    }

    _FI float FastSqrt(float x) {
        union { int i; float x; } u; u.x = x;
        u.i = (1 << 29) + (u.i >> 1) - (1 << 22);
        return u.x;
    }

    _FI float FastCos(float angle) {
        angle = angle - floorf(angle * 0.15f) * 6.28f;
        angle = angle > 0.f ? angle : -angle;
        if (angle < 1.57f) return CosAdd(angle);
        if (angle < 3.14f) return -CosAdd(3.14f - angle);
        if (angle < 4.71f) return -CosAdd(angle - 3.14f);
        return CosAdd(6.28f - angle);
    }

    _FI float FastSin(float angle) {
        return FastCos(1.57f - angle);
    }

    //setup
    void Init(int Width, int Height, const wchar_t* FontName, int FontSize, int Scale)
    {
        //create dc
        ScreenDC = EPtr(CallPtr<HDC>(GetWin32k(E("NtGdiCreateCompatibleDC")), nullptr));

        //alloc usermode buff
        UserBuffer = EPtr(UAlloc(4096));

        //create bitmap (normal size)
        PBITMAPINFO InfoUser = (PBITMAPINFO)EPtr(UserBuffer);
        InfoUser->bmiHeader.biSize = 40;
        InfoUser->bmiHeader.biWidth = Width;
        InfoUser->bmiHeader.biHeight = -Height;
        InfoUser->bmiHeader.biPlanes = 1;
        InfoUser->bmiHeader.biBitCount = 32;
        InfoUser->bmiHeader.biCompression = 0;
        InfoUser->bmiHeader.biSizeImage = Width * Height * 4;
        PVOID* MappedTextureUser = (PVOID*)((ULONG64)InfoUser + 0x800);
        PVOID NtGdiCreateDIBSection_Fn = GetWin32k(E("NtGdiCreateDIBSection"));
        ScreenBitmap = EPtr(CallPtr<HBITMAP>(NtGdiCreateDIBSection_Fn, EPtr(ScreenDC), nullptr, 0, InfoUser, 0, 40, 0, 0, MappedTextureUser));
        MappedTexture = EPtr((PBYTE)*MappedTextureUser);

        //get w32k base (for resolve many functions)
        auto win32kFull = GetKernelModuleBase(E("win32kfull.sys"));

        //need scale
        if (Scale > 1)
        {
            //create bitmap (normal size * scale)
            InfoUser->bmiHeader.biWidth = (Width * Scale);
            InfoUser->bmiHeader.biHeight = -(Height * Scale);
            InfoUser->bmiHeader.biSizeImage = (Width * Scale) * (Height * Scale) * 4;
            ScaleScreenBitmap = EPtr(CallPtr<HBITMAP>(NtGdiCreateDIBSection_Fn, EPtr(ScreenDC), nullptr, 0, InfoUser, 0, 40, 0, 0, MappedTextureUser));
            ScaleMappedTexture = EPtr((PBYTE)*MappedTextureUser);

            //GreSetStretchBltMode (FIX MIXING COLORS, SLOW)
            auto GreSetStretchBltMode = (PVOID)RVA(FindPatternSect(win32kFull, E(".text"), E("E8 ? ? ? ? 48 8B D7 89 84")), 5);
            CallPtr(GreSetStretchBltMode, EPtr(ScreenDC), HALFTONE);
        }

        //cleanup
        MemZero(InfoUser, 0x808/*https://en.wikipedia.org/wiki/808_Mafia*/);

        //select backbuffer
        SelectBitMapInternal(EPtr(((Scale > 1) ? ScaleScreenBitmap : ScreenBitmap)));

        //create font desc
        ENUMLOGFONTEXDVW EnumFont; 
        MemZero(&EnumFont, sizeof(ENUMLOGFONTEXDVW));
        EnumFont.elfEnumLogfontEx.elfLogFont.lfWeight = FW_MEDIUM;
        EnumFont.elfEnumLogfontEx.elfLogFont.lfHeight = FontSize * Scale;
        MemCpy(&EnumFont.elfEnumLogfontEx.elfLogFont.lfFaceName, (PVOID)FontName, (StrLen(FontName) + 1) * 2);

        //create & select font
        CurFont = EPtr(CallPtr<HFONT>(GetWin32k(E("hfontCreate")), &EnumFont, 0ull, 0ull, 0ull, 0ull));
        SelectFontInternal(EPtr(CurFont));

        //fix text alpha
        PVOID GreSetBkMode = (PVOID)RVA(FindPatternSect(win32kFull, E(".text"), E("E8 ? ? ? ? 89 45 7F 85")), 5);
        CallPtr(GreSetBkMode, EPtr(ScreenDC), TRANSPARENT);

        //fixup solid brush
        DcAttrObject = EPtr(GetDCObjectInternal());
        CallPtr<HBRUSH>(GetWin32k(E("NtGdiSelectBrush")), EPtr(ScreenDC), GetStockObjectInternal(DC_BRUSH));
        
        //save vars
        ScrScale = Scale;
        CurrentWidth = Width;
        CurrentHeight = Height;
    }

public:
    //mgr
    _FI void NewFrame(int Width, int Height, const wchar_t* FontName, int FontSize, int ExtraSample = 1)
    {
        //need init/reinit
        if ((Width != CurrentWidth) || (Height != CurrentHeight)) 
        {
            //cleanup
            Release();

            //setup render
            Init(Width, Height, FontName, FontSize, ExtraSample);
        }
    }

    void EndFrame(PBYTE Buffer)
    {
        //apply extrasampling
        if (ScrScale > 1)
        {
            //select original size bitmap
            auto SBitMap = SelectBitMapInternal(EPtr(ScreenBitmap));

            //NtGdiStretchDIBitsInternal
            static PVOID NtGdiStretchDIBitsInternal_Fn = nullptr;
            if (!NtGdiStretchDIBitsInternal_Fn) {
                NtGdiStretchDIBitsInternal_Fn = EPtr(GetWin32k(E("NtGdiStretchDIBitsInternal")));
            }

            //create desc bitmap
            PBITMAPINFO InfoUser = (PBITMAPINFO)EPtr(UserBuffer);
            InfoUser->bmiHeader.biSize = 40;
            InfoUser->bmiHeader.biPlanes = 1;
            InfoUser->bmiHeader.biBitCount = 32;
            InfoUser->bmiHeader.biWidth = (CurrentWidth * ScrScale);
            InfoUser->bmiHeader.biHeight = -(CurrentHeight * ScrScale);
            InfoUser->bmiHeader.biSizeImage = (CurrentWidth * ScrScale) * (CurrentHeight * ScrScale) * 4;
            CallPtr(EPtr(NtGdiStretchDIBitsInternal_Fn), EPtr(ScreenDC), 0, 0, CurrentWidth, CurrentHeight, 0, 0,
                CurrentWidth * ScrScale, CurrentHeight * ScrScale, EPtr(ScaleMappedTexture), InfoUser, 0, SRCCOPY, 40, InfoUser->bmiHeader.biSizeImage, 0ull);
            
            //cleanup & restore bitmap
            MemZero(InfoUser, sizeof(BITMAPINFO));
            SelectBitMapInternal(SBitMap);
        }
        
        //fix alpha & copy & clear buffer (BUG: no black color)
        auto MappedTextureDecrt = EPtr(MappedTexture);
        for (ULONG i = 0; i < CurrentWidth * CurrentHeight * 4; i += 8/*2 pixels*/)
        {
            //copy pixels
            ULONG Pixel1 = *(ULONG*)&Buffer[i] = *(ULONG*)&MappedTextureDecrt[i];
            ULONG Pixel2 = *(ULONG*)&Buffer[i + 4] = *(ULONG*)&MappedTextureDecrt[i + 4];

            //fix alpha
            if (Pixel1) {
                Buffer[i + 3] = (char)((float)Buffer[i] * 0.34f + (float)Buffer[i + 1] * 0.55f + (float)Buffer[i + 2] * 0.11f);
            }

            if (Pixel2) {
                Buffer[i + 7] = (char)((float)Buffer[i + 4] * 0.34f + (float)Buffer[i + 5] * 0.55f + (float)Buffer[i + 6] * 0.11f);
            }

            //reset pixels
            *(ULONG*)&MappedTextureDecrt[i] = 0;
            *(ULONG*)&MappedTextureDecrt[i + 4] = 0;
        }
    }

    void Release() 
    {
        //if need
        if (ScreenDC)
        {
            //remove bitmaps
            SelectBitMapInternal((HBITMAP)GetStockObjectInternal(21/*DEFAULT_BITMAP*/));
            RemoveObjInternal(EPtr(ScreenBitmap));
            RemoveObjInternal(EPtr(ScaleScreenBitmap));
            
            //remove font
            SelectFontInternal((HFONT)GetStockObjectInternal(SYSTEM_FONT));
            RemoveObjInternal(EPtr(CurFont));

            //release userbuff
            UFree(EPtr(UserBuffer));

            //remove screen dc
            RemoveObjInternal(EPtr(ScreenDC));
            ScreenDC = nullptr;
        }
    }

    //render line
    void Line(int x0, int y0, int x1, int y1, COLORREF Color, int Thick = 1)
    {
        //apply scale
        x0 *= ScrScale;
        y0 *= ScrScale;
        x1 *= ScrScale;
        y1 *= ScrScale;
        Thick *= ScrScale;

        //gen dots
        POINT Dots[2];
        Dots[0] = { x0, y0 };
        Dots[1] = { x1, y1 };

        //draw polyline
        PolyLineInternal(Dots, 2, Thick, Color);
    }

    //render circle
    void Circle(int x, int y, COLORREF Color, float Radius, int Thick = 1)
    {
        //apply scale
        x *= ScrScale;
        y *= ScrScale;
        Thick *= ScrScale;
        Radius *= ScrScale;

        //gen dots
        POINT Dots[120]; int NumDots = 0;
        for (float i = 0.f; i < 6.28f; i += .054f) {
            Dots[NumDots++] = {
                LONG(x + Radius * FastCos(i)),
                LONG(y + Radius * FastSin(i))
            };
        }

        //fix end
        Dots[NumDots] = Dots[0];

        //draw polyline
        PolyLineInternal(Dots, NumDots, Thick, Color);
    }

    void FillCircle(int x, int y, COLORREF Color, float Radius)
    {
        //apply scale
        x *= ScrScale;
        y *= ScrScale;
        Radius *= ScrScale;

        //gen dots
        POINT Dots[120]; int NumDots = 0;
        for (float i = 0.f; i < 6.28f; i += .054f) {
            Dots[NumDots++] = {
                LONG(x + Radius * FastCos(i)),
                LONG(y + Radius * FastSin(i))
            };
        }

        //draw polygon
        PolygonInternal(Dots, NumDots, Color);
    }

    //render rectangle
    void Rectangle(int x, int y, int w, int h, COLORREF Color, int Thick = 1)
    { 
        //apply scale
        x *= ScrScale;
        y *= ScrScale;
        w *= ScrScale;
        h *= ScrScale;
        Thick *= ScrScale;

        //gen dots
        POINT Dots[5];
        Dots[0] = { x, y };
        Dots[1] = { x + w, y };
        Dots[2] = { x + w, y + h };
        Dots[3] = { x, y + h };
        Dots[4] = Dots[0];

        //draw polyline
        PolyLineInternal(Dots, 5, Thick, Color);
    }
    
    void RoundedRectangle(int x, int y, int w, int h, COLORREF Color, float Radius, int Thick = 1) //shit
    {
        //apply scale
        x *= ScrScale;
        y *= ScrScale;
        w *= ScrScale;
        h *= ScrScale;
        Thick *= ScrScale;
        Radius *= ScrScale;

        //gen dots
        POINT Add{};
        POINT Dots[25];
        for (int i = 0; i < 24; ++i)
        {
            //gen dot
            float angle = (float(i) / -24.f) * 6.28f - (6.28f / 16.f);
            Dots[i].x = Radius + x + Add.x + (Radius * FastSin(angle));
            Dots[i].y = h - Radius + y + Add.y + (Radius * FastCos(angle));

            //calc offset
            if (i == 4) { Add.y = -h + (Radius * 2.f); }
            else if (i == 10) { Add.x = w - (Radius * 2.f); }
            else if (i == 16) Add.y = 0.f; else if (i == 22) Add.x = 0.f;
        }

        //fix end
        Dots[24] = Dots[0];

        //Line(Dots[16].x, Dots[16].y, Dots[22].x, Dots[22].y, RGB(255, 0, 0));

        //draw polyline
        PolyLineInternal(Dots, 25, Thick, Color);
    }

    void FillRectangle(int x, int y, int w, int h, COLORREF Color)
    {
        //apply scale
        x *= ScrScale;
        y *= ScrScale;
        w *= ScrScale;
        h *= ScrScale;

        //gen dots
        POINT Dots[4];
        Dots[0] = { x, y };
        Dots[1] = { x + w, y };
        Dots[2] = { x + w, y + h };
        Dots[3] = { x, y + h };

        //draw polygon
        PolygonInternal(Dots, 4, Color);
    }

    void FillRoundedRectangle(int x, int y, int w, int h, COLORREF Color, float Radius)
    {
        //apply scale
        x *= ScrScale;
        y *= ScrScale;
        w *= ScrScale;
        h *= ScrScale; 
        Radius *= ScrScale;

        //gen dots
        POINT Add{};
        POINT Dots[24];
        for (int i = 0; i < 24; ++i)
        {
            //gen dot
            float angle = (float(i) / -24.f) * 6.28f - (6.28f / 16.f);
            Dots[i].x = Radius + x + Add.x + (Radius * FastSin(angle));
            Dots[i].y = h - Radius + y + Add.y + (Radius * FastCos(angle));

            //calc offset
            if (i == 4) { Add.y = -h + (Radius * 2.f); }
            else if (i == 10) { Add.x = w - (Radius * 2.f); }
            else if (i == 16) Add.y = 0.f;
            else if (i == 22) Add.x = 0.f;
        }
        
        //draw polygon
        PolygonInternal(Dots, 24, Color);
    }

    //render string
    void String(int x, int y, const wchar_t* String, UINT Align = TA_LEFT, COLORREF Color = RGB(255, 255, 255)) 
    {
        //apply scale
        x *= ScrScale;
        y *= ScrScale;

        //resolve functions
        static PVOID GreSetTextAlign = nullptr;
        static PVOID GreSetTextColor = nullptr;
        static PVOID GreExtTextOutWInternal = nullptr;
        if (!GreSetTextColor || !GreSetTextAlign || !GreExtTextOutWInternal) {
            auto win32kFull = GetKernelModuleBase(E("win32kfull.sys"));
            GreSetTextAlign = (PVOID)EPtr(RVA(FindPatternSect(win32kFull, E(".text"), E("E8 ? ? ? ? 89 5C 24 48 45")), 5));
            GreExtTextOutWInternal = (PVOID)EPtr(RVA(FindPatternSect(win32kFull, E(".text"), E("E8 ? ? ? ? 49 83 C6 02 BB")), 5));
            GreSetTextColor = (PVOID)EPtr(RVA(FindPatternSect(win32kFull, E(".text"), E("E8 ? ? ? ? BA ? ? ? ? 89 44 24 70")), 5));
        }

        //draw text
        auto hDC = EPtr(ScreenDC);
        CallPtr(EPtr(GreSetTextAlign), hDC, Align);
        CallPtr(EPtr(GreSetTextColor), hDC, Color);
        CallPtr(EPtr(GreExtTextOutWInternal), hDC, x, y, 0ull, 0ull, String, StrLen(String), 0ull, 0ull);
    }

    SIZE TextRect(const wchar_t* String)
    {
        //resolve function
        static PVOID GreGetTextExtentW = nullptr;
        if (!GreGetTextExtentW) {
            auto win32kFull = GetKernelModuleBase(E("win32kfull.sys"));
            GreGetTextExtentW = (PVOID)EPtr(RVA(FindPatternSect(win32kFull, E(".text"), E("E8 ? ? ? ? 8B D8 41 83 FD 10")), 5));
        }
       
        //get text size
        SIZE TextRect{};
        CallPtr(EPtr(GreGetTextExtentW), EPtr(ScreenDC), String, StrLen(String), &TextRect, 1);
        return SIZE{ TextRect.cx / ScrScale, TextRect.cy / ScrScale }; //fix scale
    }
} rctx;
