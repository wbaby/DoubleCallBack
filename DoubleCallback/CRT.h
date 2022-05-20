template <typename StrType, typename StrType2>
inl bool StrICmp(StrType Str, StrType2 InStr, bool Two) 
{
	#define ToLower(Char) ((Char >= 'A' && Char <= 'Z') ? (Char + 32) : Char)

	if (!Str || !InStr) 
		return false;

	wchar_t c1, c2; do {
		c1 = *Str++; c2 = *InStr++;
		c1 = ToLower(c1); c2 = ToLower(c2);
		if (!c1 && (Two ? !c2 : 1))
			return true;
	} while (c1 == c2);

	return false;
}

template <typename StrType> 
inl int StrLen(StrType Str) {
	if (!Str) return 0;
	StrType Str2 = Str;
	while (*Str2) *Str2++;
	return (int)(Str2 - Str);
}

inl void MemCpy(PVOID Dst, PVOID Src, ULONG Size) {
	__movsb((PUCHAR)Dst, (const PUCHAR)Src, Size);
}

inl void MemZero(PVOID Ptr, SIZE_T Size, UCHAR Filling = 0) {
	__stosb((PUCHAR)Ptr, Filling, Size);
}