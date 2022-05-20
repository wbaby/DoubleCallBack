#pragma once
if (st.WaitLockFrame())
{
	auto scrSz = csgoProc.Read<POINT>(Off.dwScreenSize);

	struct
	{
		DWORD renderCommand;
		int x0;
		int y0;
		int x1;
		int y1;
		float u0;
		float v0;
		float u1;
		float v1;
		float uk4;
		DWORD colorStart;
		DWORD colorEnd; // unused if gradient direction is set to none
		DWORD gradientDirection;
		DWORD textureId;
	} drawTexturedRect =
	{
	3, // render command
	0,
	0,
	scrSz.x,
	scrSz.y,
	0.0f,
	0.0f,
	1.0f,
	1.0f,
	1.0f,
	0xFFFFFFFF,
	0xFFFFFFFF,
	3, // none
	1337
	};
	st.PutCmd(&drawTexturedRect, sizeof(drawTexturedRect));

	BR.Setup(scrSz.x, scrSz.y, rBuff);
	BR.Clear();
	//BR.FillRectangle({ 100,100 }, { 100,100 }, FColor(255, 0, 0));
	BR.String({ 5,5 }, L"//FACE", 0);


	//rctx.NewFrame(w, h, E(L"Calibri"), 48, 1);
	//rctx.String(100, 100, L"FACE", TA_CENTER, RGB(255, 0, 0));
	//rctx.EndFrame((PBYTE)pBuff111);

	st.PutCmd((PBYTE)rBuff, scrSz.x * scrSz.y * 4);
	struct
	{
		DWORD renderCommand;
		DWORD textureId;
		DWORD version;
		BOOL fullUpdate;
		DWORD size;
		DWORD width;
		DWORD height;
		DWORD x;
		DWORD y;
	} loadTexture =
	{
		1, // render command
		1337, // id
		0,//++textureVersion,
		1, // full update
		scrSz.x * scrSz.y * 4, // size
		scrSz.x, // width
		scrSz.y, // height
		0,
		0
	};

	// command
	st.PutCmd(&loadTexture, sizeof(loadTexture));
	st.ReleaseFrame();


	//sp("frame");
	//st.ReleaseFrame();
}
