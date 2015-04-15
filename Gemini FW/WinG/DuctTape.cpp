#include "stdafx.h"

#ifdef DEBUG
#include "DuctTape.h"

#include "dbghelp.h"
#pragma comment(lib, "dbghelp.lib" )

#include "Gem/String/WStr.h"
#include "Gem/String/DebugOutput.h"
#include "Gem/ConCur/CritSec.h"


#define OUTPUT(a) DebugOutput()<<a

namespace Gem { namespace DuctTape {

	class SymbolMan { //loads and manages symbol data
	public:
		SymbolMan() : ProcessHndl(GetCurrentProcess()) {	
			if(!SymbolsLoaded ) loadSymbols();

			SYMBOL_INFO *symbolData = (SYMBOL_INFO*)&SymbolData;
			symbolData->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbolData->MaxNameLen = SymbolMaxNameLen;
			FileLineData.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		}
		void printCallStack( const SIZE_T callStack[], const u32 &callStackCount ) {
			SYMBOL_INFO *symbolData = (SYMBOL_INFO*)&SymbolData;
			for(u32 i = 0; i < callStackCount; i++) {
				if( SymFromAddr(ProcessHndl, callStack[i], NULL, symbolData)) 
					OUTPUT( "func := "<<symbolData->Name);
				else
					OUTPUT( "Failed to find function name!");			
				DWORD dwDisplacement;
				if( SymGetLineFromAddr64(ProcessHndl, callStack[i], &dwDisplacement, &FileLineData ) ) {
					OUTPUT("  Line := "<<FileLineData.LineNumber<<"  File := "<<FileLineData.FileName<<"\n");
				} else {
					OUTPUT("  File and line number not available.  iPtr := "<<(void*)callStack[i]<<"\n");
				}
			}
		}
	private:
		static BOOL __stdcall loadedModuleCB( PCWSTR modName, DWORD64 modAddr, ULONG modSize, PVOID context ) {
			HANDLE hProcess = (HANDLE)context;

			WStr<MAX_PATH> modName2( modName );
	
			IMAGEHLP_MODULEW64  moduleimageinfo;  moduleimageinfo.SizeOfStruct = sizeof(moduleimageinfo);
			if( SymGetModuleInfoW64(hProcess, modAddr,&moduleimageinfo) != TRUE) {
				SymLoadModule64(hProcess, NULL, modName2, NULL, modAddr, modSize);
				if( SymGetModuleInfoW64(hProcess, modAddr,&moduleimageinfo) != TRUE) 
					OUTPUT( "Failed to load symbols for "<<modName2<<"\n");
			}
			return TRUE;
		}
		static void loadSymbols() { 
			WStrW<MAX_PATH*4> pathList;
			pathList.appendFnc( GetModuleFileNameW, GetModuleHandle(NULL) );
			pathList.fileToDir();

			pathList += CSTRW( L";.\\" ); //working directory
			(pathList += L';').appendFnc( GetWindowsDirectoryW );
			(pathList += L';').appendFnc( GetSystemDirectoryW );
			(pathList += L';').appendFnc( GetEnvironmentVariableW, (LPCWSTR)"_NT_ALT_SYMBOL_PATH" );
			(pathList += L';').appendFnc( GetEnvironmentVariableW, (LPCWSTR)"_NT_SYMBOL_PATH" );

			HANDLE hProcess = GetCurrentProcess();
			SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
			AssertOnFail( SymInitializeW(hProcess, pathList, FALSE), "DuctTape:: Failed to load symbols." );
			EnumerateLoadedModulesW64( hProcess, loadedModuleCB, hProcess );
			SymbolsLoaded = true;
		}
		const HANDLE		ProcessHndl;
		static const u32	SymbolMaxNameLen = 256;
		u8					SymbolData[sizeof(SYMBOL_INFO)+SymbolMaxNameLen-1];
		IMAGEHLP_LINE64		FileLineData;	
		static bool			SymbolsLoaded;
	};
	bool SymbolMan::SymbolsLoaded = false;

	CritSec Lock;
	//be careful with any heap functions in here
	class HeapBlock{
	public:
		class InitData : Uncopyable {
		public:
#if defined(_M_IX86) || defined (_M_X64) //Creates variable 'a' storing the frame pointer 
#define FRAMEPOINTER(a) \
	SIZE_T* a; \
	{__asm mov a, Ebp }0
#else
#error "Unhandled"
#endif
			//walking the callstack walking the green callstack, also we will work out some sizes
			InitData( const size_t &reqSize ) : Count(0) {	
				
				/*static bool tFlag = true;
				if( !tFlag ) {  //guard against infinite recursion, for instance for when dumbass's like myself output to the console in here
					calcSize(reqSize);
					return; 
				} tFlag = false; */

				FRAMEPOINTER(curFP);

				for( SIZE_T* nextFP = (SIZE_T*)*curFP; nextFP != NULL; curFP = nextFP, nextFP = (SIZE_T*)*curFP ) {
					if( nextFP < curFP )							break; // TODO -handle error :: Frame pointer addresses should always increase as we move up the stack.
					if( (SIZE_T)*curFP & (sizeof(SIZE_T*) - 1) )	break; // TODO -handle error :: Frame pointer addresses should always be aligned to the size of a pointer. This probably means that 
																			// --   we've encountered a frame that was created by a module built with frame pointer omission (FPO) optimization turned on
					if( IsBadReadPtr( nextFP, sizeof(SIZE_T*) ) )	break; // TODO -handle error :: Bogus frame pointer. Again, this probably means that we've encountered a frame built with FPO optimization.
					FrameAddr[Count] = *(curFP + 1);			
					if( Count++ == MaxCount ) break;
				}
				calcSize(reqSize);
			}
			const u32& RequiredMemory() const { return TotalSize; }
		private: friend class HeapBlock;
			SIZE_T*		callStackAddr( HeapBlock* hb ) const { return (SIZE_T*)((u8*)hb+TotalSize-Count*sizeof(SIZE_T)); }
			void		copyCallStack( HeapBlock* hb ) const { memcpy( callStackAddr(hb), FrameAddr, Count*sizeof(SIZE_T) ); }
			void		calcSize( const size_t &reqSize ) { TotalSize = sizeof(HeapBlock) + reqSize + Count*sizeof(SIZE_T); }
			u32 Count;
			static const u32 MaxCount = 32;
			SIZE_T FrameAddr[MaxCount];
			size_t TotalSize;
		};  

		static HeapBlock*	realPtr( const ptr &p )			{ HeapBlock* ret = ((HeapBlock*)p)-1; ret->assertValidity(); return ret; }
		static ptr			init( const ptr &p, const InitData &id ) {
			HeapBlock *t = (HeapBlock*)p;
			new (t) HeapBlock(id);
			return t->retPtr();
		}
		static ptr			update( const ptr &p, const HeapBlock *const &oldP, const InitData &id ) {
			HeapBlock *t = (HeapBlock*)p;
			if( t != oldP ) {
				t->fix(id);
			} else 	t->Size = id.RequiredMemory(); //leave the callstack as is, err.. easier, other wise we may have to realllocate
			return t->retPtr();
		}
		static ptr			deinit( const ptr &p ) {
			HeapBlock *t = realPtr(p);
			t->~HeapBlock();
			return t;
		}
		static void			printAll() { 

			if(Last) {
				SymbolMan sm;
				for( HeapBlock* h = Last; h; h = h->Prev ) h->print(sm); 
				Beep( 8000, 400 );
				Beep( 4000, 500 );
			}
		}

	private:
		HeapBlock( const InitData &id ) : Size(id.RequiredMemory()), Next(0), Prev(Last), MagicNo(MagicNumber) { 
			initCallStack(id);
			//PRINT("Initing "<<this<<"  prev "<<Prev<<"\n");
			if(Prev) {
				Prev->assertValidity();
				Prev->Next = this;  
			}
			Last = this; 
		}
		void initCallStack( const InitData &id ) {
			CallStackCount = id.Count;
			CallStack = id.callStackAddr(this);
			id.copyCallStack(this);
		}
		~HeapBlock() {
			//IF_RECORDUNLINKING(UnlinkFile <<this<<"  -- "<<(void*)(((u8*)this)+Size+sizeof(HeapBlock))<<"  F "<<File<< " L "<<Line<<"\n");

			//if(Prev) Prev->CheckP(this);
			//if(Next) Next->CheckN(this);

			if(Next) {
				Next->Prev = Prev; 
			} else {
				Last = Prev;
			}
			if(Prev) {
				Prev->Next = Next;
			}
			//Next = Prev = 0;
			//MagicNo = 0;
		}
		void fix( const InitData &id ) {
			if( Next ) 
				Next->Prev = this;
			else 
				Last = this;
			if( Prev ) Prev->Next = this;
			initCallStack(id);
			assertValidity();
		}
		void assertValidity() { //not exactly foolproof...but debug allocaters do their stuff too..
			Assert( MagicNo == MagicNumber,				"HeapBlock:: 'this' is corrupted" );
			if( Prev ) {
				Assert( Prev->MagicNo == MagicNumber,	"HeapBlock:: 'Prev' is corrupted" );
				Assert( Prev->Next == this,				"HeapBlock:: 'Prev' is corrupted" );
			}
			if( Next ) {
				Assert( Next->MagicNo == MagicNumber,	"HeapBlock:: 'Next' is corrupted" );
				Assert( Next->Prev == this,				"HeapBlock:: 'Next' is corrupted" );
			} else {
				Assert( Last == this,					"HeapBlock:: has no 'Next' yet 'Last != this'" );
			}
		} 
		void	print( SymbolMan &sm ) { 		
			OUTPUT(" HeapBlock @ "<<this<<"  Size "<<Size<<"  CallStackCount "<<CallStackCount<<"\n"); 
			sm.printCallStack( CallStack, CallStackCount );
		}
		ptr		retPtr()			{ return this+1; }

		
		u64 MagicNo;

		size_t Size;
		SIZE_T* CallStack;
		u32 CallStackCount;

		HeapBlock *Next, *Prev;
		static const size_t DesSizeOf = 32; //neat alignment
		u8 pad[DesSizeOf-sizeof(u64)-sizeof(size_t)-sizeof(SIZE_T*)-sizeof(u32)-sizeof(HeapBlock*)*2];
		void zNeverToBeCalled() { CCheck(sizeof(HeapBlock)==DesSizeOf); }

		static HeapBlock* Last;
		static u64 MagicNumber;
	}; 
	HeapBlock* HeapBlock::Last = 0;
	u64	HeapBlock::MagicNumber = 13371337133713371337U ^ (size_t)&Last;

	#define TABLE _AllocWrapper_Tbl
	FARPROC TABLE[4];
	template<int i> struct AllocWrapper {
		typedef void* (__cdecl *Proc_t)(size_t);
		static void* __cdecl func( size_t s ) {
			CritSec::Handle h(Lock);
			HeapBlock::InitData id(s);
			return HeapBlock::init( ((Proc_t)TABLE[i])( id.RequiredMemory() ), id );
		}
		static void set( FARPROC proc )	{ 
			Assert( i < ArySize(TABLE), "Out of bounds, make the table bigger?" );
			TABLE[i] = proc; 
			static int i = 0; Assert(i++ == 0, "this function should only be called once"); 

			return;
			Proc_t test = malloc; test = func;	//compiler check, make sure Proc_t is correct
		}
	#undef TABLE
	};

	#define TABLE _AllocDbgWrapper_Tbl
	FARPROC TABLE[3];
	template<int i> struct AllocDbgWrapper {
		typedef void* (__cdecl *Proc_t)(size_t, int, const char*, int );
		static void* __cdecl func( size_t s, int b, const char *f, int l ) {
			CritSec::Handle h(Lock);
			HeapBlock::InitData id(s);
			return HeapBlock::init( ((Proc_t)TABLE[i])( id.RequiredMemory(), b, f, l ), id );
		}
		static void set( FARPROC proc )	{ 
			Assert( i < ArySize(TABLE), "Out of bounds, make the table bigger?" );
			TABLE[i] = proc; 
			static int i = 0; Assert(i++ == 0, "this function should only be called once"); 

			return;
			IF_DEBUG( Proc_t test = _malloc_dbg; test = func; ); //compiler check, make sure Proc_t is correct	
		}
	#undef TABLE
	}; 

	#define TABLE _ReallocWrapper_Tbl
	FARPROC TABLE[1];
	template<int i> struct ReallocWrapper {
		typedef void* (__cdecl *Proc_t)(void*, size_t);
		static void* __cdecl func( void * p, size_t s ) {
			CritSec::Handle h(Lock);
			HeapBlock::InitData id(s);
			if( p ) {
				HeapBlock* old = HeapBlock::realPtr( p );
				return HeapBlock::update( ((Proc_t)TABLE[i])( old, id.RequiredMemory() ), old, id );
			} else {
				return HeapBlock::init( ((Proc_t)TABLE[i])( 0, id.RequiredMemory() ), id );
			}
		}
		static void set( FARPROC proc )	{ 
			Assert( i < ArySize(TABLE), "Out of bounds, make the table bigger?" );
			TABLE[i] = proc; 
			static int i = 0; Assert(i++ == 0, "this function should only be called once"); 

			return;
			Proc_t test = realloc; test = func; //compiler check, make sure Proc_t is correct
		}
	#undef TABLE
	}; 

	#define TABLE _FreeWrapper_Tbl
	FARPROC TABLE[4];
	template<int i> struct FreeWrapper {
		typedef void (__cdecl *Proc_t)( void* );
		static void __cdecl func( void* p ) {
			if(!p) return; //Mirror usual functionality
			CritSec::Handle h(Lock);
			((Proc_t)TABLE[i])( HeapBlock::deinit(p) );
		}
		static void set( FARPROC proc )	{ 
			Assert( i < ArySize(TABLE), "Out of bounds, make the table bigger?" );
			TABLE[i] = proc; 
			static int i = 0; Assert(i++ == 0, "this function should only be called once"); 
		
			return;
			Proc_t test = free; test = func;//compiler check, make sure Proc_t is correct
		}
	#undef TABLE
	}; 

	void replaceProcImportEntry( const HMODULE &clientMod, const CStr &serviceMod, const FARPROC &oldProc, const ptr &newProc) {		
		Assume( oldProc != newProc );

		//each module (the exe and each of its dll's) that references functions or global's does so indirectly and must look up the address on a table,
		// -- here we replace 'clientMod''s entry that refers to 'oldProc' of 'serviceMod' with 'newProc'

		ULONG size;
	//    EnterCriticalSection(&imagelock); //todo multithread

		IMAGE_IMPORT_DESCRIPTOR *iid = (IMAGE_IMPORT_DESCRIPTOR*)ImageDirectoryEntryToDataEx( clientMod, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size, NULL );
	   // LeaveCriticalSection(&imagelock);
		if(iid == NULL) return; //Failed  - could not not find a table refering to serviceMod

		for(;;iid++) {
			if( !iid->OriginalFirstThunk  )										return; // Failed! - to find relavent reference table 	
			if( isEqual(serviceMod, (char*)offsetPtr(clientMod, iid->Name) ) )	break; // these are droids you are looking for
		}
		
		for( IMAGE_THUNK_DATA *itd = (IMAGE_THUNK_DATA*)offsetPtr(clientMod, iid->FirstThunk); itd->u1.Function != 0x0; itd++ ) {
			if (itd->u1.Function == (size_t)oldProc) {
				DWORD oProtect;
				VirtualProtect( &itd->u1.Function, sizeof(itd->u1.Function), PAGE_READWRITE, &oProtect ); //ensure we are allowed to overwrite table entry
				itd->u1.Function = (size_t)newProc;
				VirtualProtect( &itd->u1.Function, sizeof(itd->u1.Function), oProtect, &oProtect );
				return; //Success
			}
		}
		//Fail - Could not find 'oldProc', maybe 'clientMod' doesn't actually use it, or we have already overwriten it
	}
	template<class WrapClass>
	void WrapProc( bool procSet, const HMODULE &cMod, const HMODULE &sMod, const CStr &sModName, const CStr &procName ) {
		FARPROC proc = GetProcAddress( sMod, procName );
		Assert( proc != NULL, "Wrapping null proc??" );
		if(!procSet) WrapClass::set( proc );
		replaceProcImportEntry( cMod, sModName, proc, WrapClass::func );
	}

	bool ProcSet = false;
	void activateForModule( HMODULE mod ) {
		if(!mod) mod = GetModuleHandleA(0);
		/*
		FARPROC proc [] = {		GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "??2@YAPAXI@Z"),			// new(uint)
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "??2@YAPAXIHPBDH@Z"),		// new(uint, const char*, int)
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "??3@YAXPAX@Z"),			// delete(void*);
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "??_U@YAPAXI@Z"),			// new [] (uint)
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "??_U@YAPAXIHPBDH@Z"),	// new [] (uint, const char*, int)
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "??_V@YAXPAX@Z"),			// delete [] (void*);
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "malloc"),				// 
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "_malloc_base"),			// 
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "_malloc_dbg"),			// 
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "_calloc_dbg"),			// 
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "calloc"),				// 
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "realloc"),				// 
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "_realloc_dbg"),			// 
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "_free_base"),			// 
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "_free_dbg"),				// 
								GetProcAddress(GetModuleHandle(L"msvcr120d.dll"), "free"),					// 
		}; */
		HMODULE msvcr120d = GetModuleHandleA("msvcr120d.dll");
		WrapProc<AllocWrapper<0>>		( ProcSet, mod, msvcr120d, CSTR("msvcr120d.dll"), CSTR("malloc")			);
		WrapProc<AllocWrapper<1>>		( ProcSet, mod, msvcr120d, CSTR("msvcr120d.dll"), CSTR("_malloc_base")		);
		WrapProc<AllocWrapper<2>>		( ProcSet, mod, msvcr120d, CSTR("msvcr120d.dll"), CSTR("??2@YAPAXI@Z")		); // new(uint)
		WrapProc<AllocWrapper<3>>		( ProcSet, mod, msvcr120d, CSTR("msvcr120d.dll"), CSTR("??_U@YAPAXI@Z")		); // new [] (uint)0
		WrapProc<ReallocWrapper<0>>		( ProcSet, mod, msvcr120d, CSTR("msvcr120d.dll"), CSTR("realloc")			);
		WrapProc<FreeWrapper<0>>		( ProcSet, mod, msvcr120d, CSTR("msvcr120d.dll"), CSTR("free")				);
		WrapProc<FreeWrapper<1>>		( ProcSet, mod, msvcr120d, CSTR("msvcr120d.dll"), CSTR("_free_base")		);
		WrapProc<FreeWrapper<2>>		( ProcSet, mod, msvcr120d, CSTR("msvcr120d.dll"), CSTR("??3@YAXPAX@Z")		); // delete(void*);
		WrapProc<FreeWrapper<3>>		( ProcSet, mod, msvcr120d, CSTR("msvcr120d.dll"), CSTR("??_V@YAXPAX@Z")		); // delete [] (void*);
		WrapProc<AllocDbgWrapper<0>>	( ProcSet, mod, msvcr120d, CSTR("msvcr120d.dll"), CSTR("_malloc_dbg")		);
		WrapProc<AllocDbgWrapper<1>>	( ProcSet, mod, msvcr120d, CSTR("msvcr120d.dll"), CSTR("??2@YAPAXIHPBDH@Z")	); // new(uint, const char*, int)
		WrapProc<AllocDbgWrapper<2>>	( ProcSet, mod, msvcr120d, CSTR("msvcr120d.dll"), CSTR("??_U@YAPAXIHPBDH@Z")); // new [] (uint, const char*, int)
		ProcSet = true;

	}

	void activateForModule( const char *name ) { activateForModule( GetModuleHandleA(name) );  }
	void deactivateForModule( const char *name );

	void printAll() { HeapBlock::printAll();  }
}	}

#endif //DEBUG