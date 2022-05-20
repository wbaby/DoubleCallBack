#include "font.h"

//class Vector2 {
//public:
//	float x;
//	float y;
//	__forceinline Vector2() {
//		x = y = 0;
//	}
//
//	__forceinline Vector2(float X, float Y) {
//		x = X; y = Y;
//	}
//	__forceinline Vector2 operator/(float v) const {
//		return Vector2(x / v, y / v);
//	}
//	__forceinline Vector2& operator-=(const Vector2& v) {
//		x -= v.x; y -= v.y; return *this;
//	}
//};

class FColor
{
public:
	uint8_t RGBA[4];

	__forceinline FColor(uint8_t R, uint8_t G, uint8_t B, uint8_t A = 255) {
		RGBA[0] = R; RGBA[1] = G; RGBA[2] = B; RGBA[3] = A;
	}

	__forceinline FColor(uint32_t val) {
		*(uint32_t*)&RGBA = val;
	}

	__forceinline auto Get() const {
		return *(uint32_t*)&RGBA;
	}
};

class ByteRender
{
private:
	int w, h;
	bool align;
	uint32_t* Arr;
	struct CharDesc {
		short srcX, srcY;
		short srcW, srcH;
		short xOff, yOff;
		short xAdv;
	};

	//
	int OddNumber(int val) {
		return (val % 2) ? val : val + 1;
	}

	int EvenNumber(int val) {
		return (val % 2) ? val + 1 : val;
	}

	//pixel
	FColor GetPixel(int x, int y) {
		return FColor(Arr[(y * w) + x]);
	}

	FColor ColorBlend(FColor prev, FColor cur)
	{
		//pixels rgba to float
		float added[] = { cur.RGBA[0], cur.RGBA[1], cur.RGBA[2], (float)cur.RGBA[3] / 255.f };
		float base[] = { prev.RGBA[0], prev.RGBA[1], prev.RGBA[2], (float)prev.RGBA[3] / 255.f };

		//mix colors
		float mix[4];
		mix[3] = 1 - (1 - added[3]) * (1 - base[3]); // alpha
		mix[0] = roundf((added[0] * added[3] / mix[3]) + (base[0] * base[3] * (1 - added[3]) / mix[3])); // red
		mix[1] = roundf((added[1] * added[3] / mix[3]) + (base[1] * base[3] * (1 - added[3]) / mix[3])); // green
		mix[2] = roundf((added[2] * added[3] / mix[3]) + (base[2] * base[3] * (1 - added[3]) / mix[3])); // blue

		//set new pixel color
		return FColor(mix[0], mix[1], mix[2], (mix[3] * 255.f));
	}

	void SetPixel(int x, int y, FColor color, int br = 0)
	{
		if ((x > w) || (y > h))
			return;

		//need fix color
		if (align && br)
		{
			//apply brightness
			color.RGBA[3] = 255 - br;
			auto prevPx = GetPixel(x, y);

			//need color blend
			if (prevPx.RGBA[3])
				color = ColorBlend(prevPx, color);
		}

		//apply pixels
		Arr[(y * w) + x] = color.Get();
	}

	//circle
	void plotQuadRationalBezierWidthSeg(int x0, int y0, int x1, int y1, int x2, int y2, float w, float th, FColor clr)
	{   /* plot a limited rational Bezier segment of thickness th, squared weight */
		int sx = x2 - x1, sy = y2 - y1;                  /* relative values for checks */
		int dx = x0 - x2, dy = y0 - y2, xx = x0 - x1, yy = y0 - y1;
		float xy = xx * sy + yy * sx, cur = xx * sy - yy * sx, err, e2, ed;         /* curvature */

		//assert(xx * sx <= 0.0 && yy * sy <= 0.0);  /* sign of gradient must not change */

		if (cur != 0.0 && w > 0.0) {                           /* no straight line */
			if (sx * sx + sy * sy > xx * xx + yy * yy) {              /* begin with longer part */
				x2 = x0; x0 -= dx; y2 = y0; y0 -= dy; cur = -cur;      /* swap P0 P2 */
			}
			xx = 2.0 * (4.0 * w * sx * xx + dx * dx);                 /* differences 2nd degree */
			yy = 2.0 * (4.0 * w * sy * yy + dy * dy);
			sx = x0 < x2 ? 1 : -1;                              /* x step direction */
			sy = y0 < y2 ? 1 : -1;                              /* y step direction */
			xy = -2.0 * sx * sy * (2.0 * w * xy + dx * dy);

			if (cur * sx * sy < 0) {                              /* negated curvature? */
				xx = -xx; yy = -yy; cur = -cur; xy = -xy;
			}
			dx = 4.0 * w * (x1 - x0) * sy * cur + xx / 2.0;             /* differences 1st degree */
			dy = 4.0 * w * (y0 - y1) * sx * cur + yy / 2.0;

			if (w < 0.5 && (dx + xx <= 0 || dy + yy >= 0)) {/* flat ellipse, algo fails */
				cur = (w + 1.0) / 2.0; w = sqrt(w); xy = 1.0 / (w + 1.0);
				sx = floorf((x0 + 2.0 * w * x1 + x2) * xy / 2.0 + 0.5);    /* subdivide curve  */
				sy = floorf((y0 + 2.0 * w * y1 + y2) * xy / 2.0 + 0.5);     /* plot separately */
				dx = floorf((w * x1 + x0) * xy + 0.5); dy = floorf((y1 * w + y0) * xy + 0.5);
				plotQuadRationalBezierWidthSeg(x0, y0, dx, dy, sx, sy, cur, th, clr);
				dx = floorf((w * x1 + x2) * xy + 0.5); dy = floorf((y1 * w + y2) * xy + 0.5);
				return plotQuadRationalBezierWidthSeg(sx, sy, dx, dy, x2, y2, cur, th, clr);
			}
		fail:
			for (err = 0; dy + 2 * yy < 0 && dx + 2 * xx > 0; ) /* loop of steep/flat curve */
				if (dx + dy + xy < 0) {                                   /* steep curve */
					do {
						ed = -dy - 2 * dy * dx * dx / (4. * dy * dy + dx * dx);      /* approximate sqrt */
						w = (th - 1) * ed;                             /* scale line width */
						x1 = floorf((err - ed - w / 2) / dy);              /* start offset */
						e2 = err - x1 * dy - w / 2;                   /* error value at offset */
						x1 = x0 - x1 * sx;                                  /* start point */
						SetPixel(x1, y0, clr, 255 * e2 / ed);           /* aliasing pre-pixel */
						for (e2 = -w - dy - e2; e2 - dy < ed; e2 -= dy)
							SetPixel(x1 += sx, y0, clr);              /* pixel on thick line */
						SetPixel(x1 + sx, y0, clr, 255 * e2 / ed);       /* aliasing post-pixel */
						if (y0 == y2) return;          /* last pixel -> curve finished */
						y0 += sy; dy += xy; err += dx; dx += xx;             /* y step */
						if (2 * err + dy > 0) {                            /* e_x+e_xy > 0 */
							x0 += sx; dx += xy; err += dy; dy += yy;          /* x step */
						}
						if (x0 != x2 && (dx + 2 * xx <= 0 || dy + 2 * yy >= 0))
							if (abs(y2 - y0) > abs(x2 - x0)) goto fail;
							else break;                             /* other curve near */
					} while (dx + dy + xy < 0);                  /* gradient still steep? */
												   /* change from steep to flat curve */
					for (cur = err - dy - w / 2, y1 = y0; cur < ed; y1 += sy, cur += dx) {
						for (e2 = cur, x1 = x0; e2 - dy < ed; e2 -= dy)
							SetPixel(x1 -= sx, y1, clr);              /* pixel on thick line */
						SetPixel(x1 - sx, y1, clr, 255 * e2 / ed);       /* aliasing post-pixel */
					}
				}
				else {                                               /* flat curve */
					do {
						ed = dx + 2 * dx * dy * dy / (4. * dx * dx + dy * dy);       /* approximate sqrt */
						w = (th - 1) * ed;                             /* scale line width */
						y1 = floorf((err + ed + w / 2) / dx);              /* start offset */
						e2 = y1 * dx - w / 2 - err;                   /* error value at offset */
						y1 = y0 - y1 * sy;                                  /* start point */
						SetPixel(x0, y1, clr, 255 * e2 / ed);           /* aliasing pre-pixel */
						for (e2 = dx - e2 - w; e2 + dx < ed; e2 += dx)
							SetPixel(x0, y1 += sy, clr);              /* pixel on thick line */
						SetPixel(x0, y1 + sy, clr, 255 * e2 / ed);       /* aliasing post-pixel */
						if (x0 == x2) return;          /* last pixel -> curve finished */
						x0 += sx; dx += xy; err += dy; dy += yy;             /* x step */
						if (2 * err + dx < 0) {                           /* e_y+e_xy < 0 */
							y0 += sy; dy += xy; err += dx; dx += xx;          /* y step */
						}
						if (y0 != y2 && (dx + 2 * xx <= 0 || dy + 2 * yy >= 0))
							if (abs(y2 - y0) <= abs(x2 - x0)) goto fail;
							else break;                             /* other curve near */
					} while (dx + dy + xy >= 0);                  /* gradient still flat? */
												   /* change from flat to steep curve */
					for (cur = -err + dx - w / 2, x1 = x0; cur < ed; x1 += sx, cur -= dy) {
						for (e2 = cur, y1 = y0; e2 + dx < ed; e2 += dx)
							SetPixel(x1, y1 -= sy, clr);              /* pixel on thick line */
						SetPixel(x1, y1 - sy, clr, 255 * e2 / ed);       /* aliasing post-pixel */
					}
				}
		}
		//plotLineWidth(x0, y0, x2, y2, clr, th);            /* confusing error values  */
	}

	void plotEllipseRectWidth(int x0, int y0, int x1, int y1, float th, FColor clr)
	{               /* draw anti-aliased ellipse inside rectangle with thick line */
		auto a = abs(x1 - x0), b = abs(y1 - y0), b1 = b & 1;  /* outer diameter */
		auto a2 = a - 2 * th, b2 = b - 2 * th;                            /* inner diameter */
		auto dx = 4 * (a - 1) * b * b, dy = 4 * (b1 - 1) * a * a;                /* error increment */
		float i = a + b2, err = b1 * a * a, dx2, dy2, e2, ed;
		/* thick line correction */
		//if (th < 1.5) return plotEllipseRectAA(x0, y0, x1, y1);
		if ((th - 1) * (2 * b - th) > a * a) b2 = sqrt(a * (b - a) * i * a2) / (a - th);
		if ((th - 1) * (2 * a - th) > b * b) { a2 = sqrt(b * (a - b) * i * b2) / (b - th); th = (a - a2) / 2; }
		//if (a == 0 || b == 0) return plotLineAA(x0, y0, x1, y1, clr);
		if (x0 > x1) { x0 = x1; x1 += a; }        /* if called with swapped points */
		if (y0 > y1) y0 = y1;                                  /* .. exchange them */
		if (b2 <= 0) th = a;                                     /* filled ellipse */
		e2 = th - floorf(th); th = x0 + th - e2;
		dx2 = 4 * (a2 + 2 * e2 - 1) * b2 * b2; dy2 = 4 * (b1 - 1) * a2 * a2; e2 = dx2 * e2;
		y0 += (b + 1) >> 1; y1 = y0 - b1;                              /* starting pixel */
		a = 8 * a * a; b1 = 8 * b * b; a2 = 8 * a2 * a2; b2 = 8 * b2 * b2;

		do {
			for (;;) {
				if (err < 0 || x0 > x1) { i = x0; break; }
				i = min(dx, dy); ed = max(dx, dy);
				if (y0 == y1 + 1 && 2 * err > dx && a > b1) ed = a / 4;           /* x-tip */
				else ed += 2 * ed * i * i / (4 * ed * ed + i * i + 1) + 1;/* approx ed=sqrt(dx*dx+dy*dy) */
				i = 255 * err / ed;                             /* outside anti-aliasing */
				SetPixel(x0, y0, clr, i);
				SetPixel(x0, y1, clr, i);
				SetPixel(x1, y0, clr, i);
				SetPixel(x1, y1, clr, i);
				if (err + dy + a < dx) { i = x0 + 1; break; }
				x0++; x1--; err -= dx; dx -= b1;                /* x error increment */
			}

			for (; i < th && 2 * i <= x0 + x1; i++) {                /* fill line pixel */
				SetPixel(i, y0, clr);
				SetPixel(x0 + x1 - i, y0, clr);
				SetPixel(i, y1, clr);
				SetPixel(x0 + x1 - i, y1, clr);
			}

			while (e2 > 0 && x0 + x1 >= 2 * th) {               /* inside anti-aliasing */
				i = min(dx2, dy2); ed = max(dx2, dy2);
				if (y0 == y1 + 1 && 2 * e2 > dx2 && a2 > b2) ed = a2 / 4;         /* x-tip */
				else  ed += 2 * ed * i * i / (4 * ed * ed + i * i);                 /* approximation */
				i = 255 - 255 * e2 / ed;             /* get intensity value by pixel error */
				SetPixel(th, y0, clr, i);
				SetPixel(x0 + x1 - th, y0, clr, i);
				SetPixel(th, y1, clr, i);
				SetPixel(x0 + x1 - th, y1, clr, i);
				if (e2 + dy2 + a2 < dx2) break;
				th++; e2 -= dx2; dx2 -= b2;                     /* x error increment */
			}
			e2 += dy2 += a2;
			y0++; y1--; err += dy += a;                                   /* y step */
		} while (x0 < x1);

		/*if (y0 - y1 <= b)
		{
			if (err > dy + a) { y0--; y1++; err -= dy -= a; }
			for (; y0 - y1 <= b; err += dy += a) { // too early stop of flat ellipses
				i = 255 * 4 * err / b1;                        // -> finish tip of ellipse
				SetPixel(x0, y0, clr, i);
				SetPixel(x1, y0++, clr, i);
				SetPixel(x0, y1, clr, i);
				SetPixel(x1, y1--, clr, i);
			}
		}*/
	}

	//line
	void plotLineWidth(int x0, int y0, int x1, int y1, FColor clr, float th)
	{                              /* plot an anti-aliased line of width th pixel */
		auto dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
		auto dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
		float err, e2 = sqrt(dx * dx + dy * dy);                            /* length */

		//if (th <= 1 || e2 == 0) return plotLineAA(x0, y0, x1, y1, clr);    /* assert */
		dx *= 255 / e2; dy *= 255 / e2; th = 255 * (th - 1);               /* scale values */

		if (dx < dy) {                                               /* steep line */
			x1 = round((e2 + th / 2) / dy);                          /* start offset */
			err = x1 * dy - th / 2;                  /* shift error value to offset width */
			for (x0 -= x1 * sx; ; y0 += sy) {
				SetPixel(x1 = x0, y0, clr, err);                  /* aliasing pre-pixel */
				for (e2 = dy - err - th; e2 + dy < 255; e2 += dy)
					SetPixel(x1 += sx, y0, clr);                      /* pixel on the line */
				SetPixel(x1 + sx, y0, clr, e2);                    /* aliasing post-pixel */
				if (y0 == y1) break;
				err += dx;                                                 /* y-step */
				if (err > 255) { err -= dy; x0 += sx; }                    /* x-step */
			}
		}
		else {                                                      /* flat line */
			y1 = round((e2 + th / 2) / dx);                          /* start offset */
			err = y1 * dx - th / 2;                  /* shift error value to offset width */
			for (y0 -= y1 * sy; ; x0 += sx) {
				SetPixel(x0, y1 = y0, clr, err);                  /* aliasing pre-pixel */
				for (e2 = dx - err - th; e2 + dx < 255; e2 += dx)
					SetPixel(x0, y1 += sy, clr);                      /* pixel on the line */
				SetPixel(x0, y1 + sy, clr, e2);                    /* aliasing post-pixel */
				if (x0 == x1) break;
				err += dy;                                                 /* x-step */
				if (err > 255) { err -= dx; y0 += sy; }                    /* y-step */
			}
		}
	}

	//text
	bool GetCharDesc(wchar_t Char, CharDesc* Desc)
	{
		int CharID = Char;
		CharDesc* FDBuff = (CharDesc*)&FontDesc;

		if ((CharID >= 32) && (CharID <= 126)) { //latin
			*Desc = FDBuff[0 + (CharID - 32)];
			return true;
		}

		else if ((CharID >= 1040) && (CharID <= 1103)) { //cyrillic
			*Desc = FDBuff[96 + (CharID - 1040)];
			return true;
		}

		else if (CharID == 1105) { //¸
			*Desc = FDBuff[160 + (CharID - 1105)];
			return true;
		}

		else if (CharID == 1025) { //¨
			*Desc = FDBuff[95 + (CharID - 1025)];
			return true;
		}

		//no char
		return false;
	}

	Vector2 GetTextSize(const wchar_t* String, CharDesc* DescArray, int* DescSize)
	{
		wchar_t WChar;
		int DescArrayOffset = 0;
		Vector2 TextSize = { 0.f, 0.f };
		for (int i = 0; (WChar = String[i]); ++i)
		{
			//get char description
			if (GetCharDesc(WChar, &DescArray[DescArrayOffset]))
			{
				//get char size
				CharDesc CDesc = DescArray[DescArrayOffset++];
				float YSize = float(CDesc.yOff + CDesc.srcH);
				TextSize.x += (float)CDesc.xAdv;
				TextSize.y = max(YSize, TextSize.y);
			}
		}

		//save size & ret text size
		*DescSize = DescArrayOffset;
		return TextSize;
	}

public:
	//init render
	template<typename T>
	__forceinline void Setup(int Width, int Height, T pixelsArr) {
		w = Width; h = Height;
		Arr = (uint32_t*)pixelsArr;
		align = true;
	}

	__forceinline void Clear() {
		__stosb((PUCHAR)Arr, 0, w * h * 4);
	}

	//render line
	void Line(const Vector2& Start, const Vector2& End, const FColor& Color, int Thickness = 1) {
		plotLineWidth(Start.x, Start.y, End.x, End.y, Color, OddNumber(Thickness));
	}

	//render circle
	void Circle(const Vector2& Start, const FColor& Color, int Radius, int Thickness = 1) {
		auto fix = (EvenNumber(Radius) + Thickness) / 2;
		plotEllipseRectWidth(Start.x - fix, Start.y - fix, Start.x + fix, Start.y + fix, OddNumber(Thickness), Color);
	}

	void FillCircle(const Vector2& Start, const FColor& Color, int Radius) {
		auto fix = EvenNumber(Radius) / 2;
		plotEllipseRectWidth(Start.x - fix, Start.y - fix, Start.x + fix, Start.y + fix, (float)fix, Color);
	}

	//render rectangle
	void Rectangle(const Vector2& Start, const Vector2& Size, const FColor& Color, int Thickness = 1)
	{
		int fix = Thickness / 2;
		Thickness = OddNumber(Thickness);

		align = false;

		plotLineWidth(Start.x - fix, Start.y, Start.x + Size.x + fix, Start.y, Color, Thickness);
		plotLineWidth(Start.x, Start.y, Start.x, Start.y + Size.y, Color, Thickness);
		plotLineWidth(Start.x + Size.x, Start.y, Start.x + Size.x, Start.y + Size.y, Color, Thickness);
		plotLineWidth(Start.x - fix, Start.y + Size.y, Start.x + Size.x + fix, Start.y + Size.y, Color, Thickness);

		align = true;
	}

	void FillRectangle(const Vector2& Start, const Vector2& Size, const FColor& Color) {
		for (int i = 0; i < Size.y + 1; i++) {
			int off = Start.x + ((Start.y + i) * w);
			__stosb((PUCHAR)&Arr[off], Color.Get(), (Size.x + 1) * 4);
		}
	}

	void FillRoundRectangle(const Vector2& Start, const Vector2& Size, const FColor& Color, int Radius)
	{
		Radius = EvenNumber(Radius);

		FillRectangle(Vector2{ Start.x + Radius, Start.y }, Vector2{ Size.x - Radius * 2, Size.y }, Color);
		FillRectangle(Vector2{ Start.x, Start.y + Radius }, Vector2{ Size.x, Size.y - Radius * 2 }, Color);

		FillCircle(Vector2{ Start.x + Radius, Start.y + Radius }, Color, Radius * 2);
		FillCircle(Vector2{ Start.x + Size.x - Radius, Start.y + Size.y - Radius }, Color, Radius * 2);
		FillCircle(Vector2{ Start.x + Size.x - Radius, Start.y + Radius }, Color, Radius * 2);
		FillCircle(Vector2{ Start.x + Radius, Start.y + Size.y - Radius }, Color, Radius * 2);
	}

	void RoundRectangle(const Vector2& Start, const Vector2& Size, const FColor& Color, int Radius, int Thickness = 1)
	{
		Radius = EvenNumber(Radius);
		Thickness = OddNumber(Thickness);

		plotLineWidth(Start.x + Radius, Start.y, Start.x + Size.x - Radius, Start.y, Color, Thickness);
		plotLineWidth(Start.x + Size.x, Start.y + Size.y - Radius, Start.x + Size.x, Start.y + Radius, Color, Thickness);
		plotLineWidth(Start.x + Radius, Start.y + Size.y, Start.x + Size.x - Radius, Start.y + Size.y, Color, Thickness);
		plotLineWidth(Start.x, Start.y + Size.y - Radius, Start.x, Start.y + Radius, Color, Thickness);

		plotQuadRationalBezierWidthSeg(Start.x + Radius, Start.y, Start.x, Start.y, Start.x, Start.y + Radius, 1, Thickness, Color);
		plotQuadRationalBezierWidthSeg(Start.x + Size.x - Radius, Start.y, Start.x + Size.x, Start.y, Start.x + Size.x, Start.y + Radius, 1, Thickness, Color);
		plotQuadRationalBezierWidthSeg(Start.x + Radius, Start.y + Size.y, Start.x, Start.y + Size.y, Start.x, Start.y + Size.y - Radius, 1, Thickness, Color);
		plotQuadRationalBezierWidthSeg(Start.x + Size.x - Radius, Start.y + Size.y, Start.x + Size.x, Start.y + Size.y, Start.x + Size.x, Start.y + Size.y - Radius, 1, Thickness, Color);
	}

	//render text
	Vector2 String(Vector2 Start, const wchar_t* String, bool Center = false, FColor Color = FColor(255, 255, 255))
	{
		//get char desc & string size (in px)
		if (String == nullptr) return Vector2{ 0.f, 0.f };
		int CharDescSize = 0; CharDesc CharDesc[256];
		Vector2 TextSize = GetTextSize(String, CharDesc, &CharDescSize);
		if (Center) { Start -= (TextSize / 2.f); }

		auto drawTexture = [&](const Vector2& Start, uint32_t* Texture, const Vector2& TexOff, const Vector2& RectSize) {
			for (int y = 0; y < RectSize.y; y++) {
				for (int x = 0; x < RectSize.x; x++) {
					//int off1 = (Start.x + x) + ((Start.y + y) * w);
					int off2 = (TexOff.x + x) + ((TexOff.y + y) * 170);

					//if (!FColor(Texture[off2]).RGBA[0] && !FColor(Texture[off2]).RGBA[1] && !FColor(Texture[off2]).RGBA[2])
					{
						//
						auto gg = FColor(Texture[off2]);
						auto colorAvg =/* 1.f -*/ ((float)((gg.RGBA[0] + gg.RGBA[1] + gg.RGBA[2]) / 3) / 255.f);
						auto newg = FColor((float)Color.RGBA[0] * colorAvg, (float)Color.RGBA[1] * colorAvg, (float)Color.RGBA[2] * colorAvg, FColor(Texture[off2]).RGBA[3]);


						SetPixel(Start.x + x, Start.y + y, newg, 255 - FColor(Texture[off2]).RGBA[3]);

						//printf("%d\n\n", FColor(Texture[off2]).RGBA[3]);
						//Arr[off1] = Texture[off2];
					}
					//else 
					{
						//SetPixel(Start.x + x, Start.y + y, ColorBlend(Texture[off2], Color), 0);
					}
				}
			}
		};

		//draw chars
		for (int i = 0; i < CharDescSize; ++i)
		{
			//add vertex to draw list
			//auto FillChar = RenderToolKit::AddDrawCmd(4, 6);
			float srcX = (float)CharDesc[i].srcX, srcY = (float)CharDesc[i].srcY,
				srcW = (float)CharDesc[i].srcW, srcH = (float)CharDesc[i].srcH,
				xOff = (float)CharDesc[i].xOff, yOff = (float)CharDesc[i].yOff,
				xAdv = (float)CharDesc[i].xAdv;



			drawTexture({ Start.x + (xOff /** MP*/), Start.y + (yOff /** MP*/) }, (uint32_t*)&FontBytes[0], { srcX, srcY }, { srcW, srcH });



			//add vertexes
			//auto _VtxWritePtr = FillChar.VBuffer;

			//_VtxWritePtr[0] = { { Start.x + (xOff * MP), //top left
			//	Start.y + (yOff * MP) }, { srcX, srcY }, Color.DXColor };
			//
			//_VtxWritePtr[1] = { { Start.x + ((xOff + srcW) * MP), //upper right
			//	Start.y + (yOff * MP) }, { srcX + srcW, srcY }, Color.DXColor };
			//
			//_VtxWritePtr[2] = { { Start.x + (xOff * MP), //bottom left
			//	Start.y + ((yOff + srcH) * MP) }, { srcX, srcY + srcH }, Color.DXColor };
			//
			//_VtxWritePtr[3] = { { Start.x + ((xOff + srcW) * MP), //bottom right
			//	Start.y + ((yOff + srcH) * MP) }, { srcX + srcW, srcY + srcH }, Color.DXColor };

			//add indexes
			//FillChar.IBuffer[0] = 0; FillChar.IBuffer[1] = 1; FillChar.IBuffer[2] = 2;
			//FillChar.IBuffer[3] = 3; FillChar.IBuffer[4] = 1; FillChar.IBuffer[5] = 2;

			//set char spacing
			Start.x += (xAdv /** MP*/);
		}

		return TextSize;
	}
};