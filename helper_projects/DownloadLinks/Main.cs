using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Text;
using System.Text.RegularExpressions;


namespace Uber.DownloadLinks
{
    class App
    {
        public static void Main()
        {
            if(Debugger.IsAttached)
            {
                RealMain();
                return;
            }

            try
            {
                RealMain();
            }
            catch(Exception exception)
            {
                Console.WriteLine("Caught an exception: " + exception.Message);
                Console.WriteLine("Stack trace:");
                Console.WriteLine(exception.StackTrace);
            }
        }

        private static void RealMain()
        {
            var stringReceiver = new CURLStringReceiver();
            CURL.Init();
            try
            {
                CURL.SetLongOption(CURL.LongOption.SSL_VerifyHost, 1);
                CURL.SetLongOption(CURL.LongOption.SSL_VerifyPeer, 1);
                CURL.SetStringBlobOption(CURL.StringBlobOption.CertificateAuthorityInfo, UpdateWebsite.Certificate);
                CURL.SetWriteCallback(stringReceiver.CurlWriteCallback);
                CURL.SetStringOption(CURL.StringOption.URL, UpdateWebsite.URL);
                CURL.Perform();

                var pageData = stringReceiver.Data;
                foreach(var link in _links)
                {
                    var regEx = new Regex("\"(" + link.RegEx + ")\"");
                    var match = regEx.Match(pageData);
                    if(!match.Success)
                    {
                        continue;
                    }

                    var downloadUrl = UpdateWebsite.URL + "/" + match.Groups[1].Value;
                    CreateRedirectionHTMLFile(link.FileName, link.Title, downloadUrl);
                }
            }
            finally
            {
                CURL.ShutDown();
            }
        }

        private class LinkInfo
        {
            public LinkInfo(string regEx, string fileName, string title)
            {
                RegEx = regEx;
                FileName = fileName;
                Title = title;
            }

            public string RegEx;
            public string FileName;
            public string Title;
        }

        private static readonly List<LinkInfo> _links = new List<LinkInfo>
        {
            new LinkInfo("udt_con_(.+)_x64.tar.bz2", "linux_console_x64.html", "UDT Linux command-line tools x64"),
            new LinkInfo("udt_con_(.+)_x86.tar.bz2", "linux_console_x86.html", "UDT Linux command-line tools x86"),
            new LinkInfo("udt_con_(.+)_x64.zip", "windows_console_x64.html", "UDT Windows command-line tools x64"),
            new LinkInfo("udt_con_(.+)_x86.zip", "windows_console_x86.html", "UDT Windows command-line tools x86"),
            new LinkInfo("udt_gui_(.+)_dll_(.+)_x64.zip", "windows_gui_x64.html", "UDT Windows GUI x64"),
            new LinkInfo("udt_gui_(.+)_dll_(.+)_x86.zip", "windows_gui_x86.html", "UDT Windows GUI x86"),
            new LinkInfo("udt_viewer_(.+)_x64.zip", "windows_viewer_x64.html", "UDT Windows 2D Viewer x64"),
            new LinkInfo("udt_viewer_(.+)_x86.zip", "windows_viewer_x86.html", "UDT Windows 2D Viewer x86"),
            new LinkInfo("udt_viewer_(.+)_x64.tar.bz2", "linux_viewer_x64.html", "UDT Linux 2D Viewer x64"),
            new LinkInfo("udt_viewer_(.+)_x86.tar.bz2", "linux_viewer_x86.html", "UDT Linux 2D Viewer x86")
        };

        private static void CreateRedirectionHTMLFile(string filePath, string title, string url)
        {
            File.WriteAllText(filePath, CreateRedirectionHTMLContent(title, url));
        }

        private static string CreateRedirectionHTMLContent(string title, string url)
        {
            return string.Format(Properties.Resources.RedirectionHTML, url, title);
        }
    }
}
