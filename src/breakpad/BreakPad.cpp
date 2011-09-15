/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "BreakPad.h"

#include <QString>

#ifndef WIN32
#include <unistd.h>

#define CRASH_REPORTER_BINARY "CrashReporter"


static bool
LaunchUploader( const char* dump_dir, const char* minidump_id, void* that, bool succeeded )
{
    // DON'T USE THE HEAP!!!
    // So that indeed means, no QStrings, no qDebug(), no QAnything, seriously!

    if ( !succeeded )
        return false;

    pid_t pid = fork();

    if ( pid == -1 ) // fork failed
        return false;
    if ( pid == 0 )
    {
        // we are the fork
        execl( CRASH_REPORTER_BINARY,
               CRASH_REPORTER_BINARY,
               dump_dir,
               minidump_id,
               minidump_id,
               (char*) 0 );

        // execl replaces this process, so no more code will be executed
        // unless it failed. If it failed, then we should return false.
        return false;
    }

    // we called fork()
    return true;
}


BreakPad::BreakPad( const QString& path )
    : google_breakpad::ExceptionHandler( path.toStdString(), 0, LaunchUploader, this, true, 0 )
{
}


#else


static bool
LaunchUploader( const wchar_t* dump_dir, const wchar_t* minidump_id, void* that, EXCEPTION_POINTERS *exinfo, MDRawAssertionInfo *assertion, bool succeeded )
{
    if ( !succeeded )
        return false;

    // DON'T USE THE HEAP!!!
    // So that indeed means, no QStrings, no qDebug(), no QAnything, seriously!

    const char* m_product_name = static_cast<BreakPad*>(that)->productName();

    // convert m_product_name to widechars, which sadly means the product name must be Latin1
    wchar_t product_name[ 256 ];
    char* out = (char*)product_name;
    const char* in = m_product_name - 1;
    do {
        *out++ = *++in; //latin1 chars fit in first byte of each wchar
        *out++ = '\0';  //every second byte is NULL
    }
    while (*in);

    wchar_t command[MAX_PATH * 3 + 6];
    wcscpy( command, CRASH_REPORTER_BINARY L" \"" );
    wcscat( command, dump_dir );
    wcscat( command, L"\" \"" );
    wcscat( command, minidump_id );
    wcscat( command, L"\" \"" );
    wcscat( command, product_name );
    wcscat( command, L"\"" );

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof( si ) );
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;
    ZeroMemory( &pi, sizeof(pi) );

    if (CreateProcess( NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
        TerminateProcess( GetCurrentProcess(), 1 );
    }

    return false;
}


BreakPad::BreakPad( const QString& path )
    : google_breakpad::ExceptionHandler( path.toStdWString(), 0, LaunchUploader, this, true )
{
}

#endif // WIN32
