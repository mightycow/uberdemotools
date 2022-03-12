using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;


namespace Uber
{
    public class UpdateWebsite
    {
        public static readonly string URL = "https://udt.playmorepromode.com";

        public static readonly string Certificate =
@"ISRG Root X1
============
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAwTzELMAkGA1UE
BhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2VhcmNoIEdyb3VwMRUwEwYDVQQD
EwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQG
EwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMT
DElTUkcgUm9vdCBYMTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54r
Vygch77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+0TM8ukj1
3Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6UA5/TR5d8mUgjU+g4rk8K
b4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sWT8KOEUt+zwvo/7V3LvSye0rgTBIlDHCN
Aymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyHB5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ
4Q7e2RCOFvu396j3x+UCB5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf
1b0SHzUvKBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWnOlFu
hjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTnjh8BCNAw1FtxNrQH
usEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbwqHyGO0aoSCqI3Haadr8faqU9GY/r
OPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CIrU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4G
A1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY
9umbbjANBgkqhkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ3BebYhtF8GaV
0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KKNFtY2PwByVS5uCbMiogziUwt
hDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJw
TdwJx4nLCgdNbOhdjsnvzqvHu7UrTkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nx
e5AW0wdeRlN8NwdCjNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZA
JzVcoyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq4RgqsahD
YVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPAmRGunUHBcnWEvgJBQl9n
JEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57demyPxgcYxn/eR44/KJ4EBs+lVDR3veyJ
m+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----

ISRG Root X2
============
-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQswCQYDVQQGEwJV
UzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElT
UkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVT
MSkwJwYDVQQKEyBJbnRlcm5ldCBTZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNS
RyBSb290IFgyMHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0H
ttwW+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9ItgKbppb
d9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNV
HQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZIzj0EAwMDaAAwZQIwe3lORlCEwkSHRhtF
cP9Ymd70/aTSVaYgLXTWNLxBo1BfASdWtL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5
U6VR5CmD1/iQMVtCnwr1/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----";
    }

    public class CURL
    {
        private enum GlobalInitFlags
        {
            Nothing = 0,
            SSL = 1,
            Win32 = 2,
            All = 3
        }

        private const string _dllPath = "libcurl.dll";

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        private struct Blob
        {
            public IntPtr Data;
            public UIntPtr Length;
            public UInt32 Flags; // set to 1 to force a copy
            public UInt32 Padding;
        }

        private enum ErrorCode
        {
            Ok = 0,
            UnsupportedProtocol = 1,
            FailedInit = 2,
            UrlMalformat = 3,
            NotBuiltIn = 4,
            CouldntResolveProxy = 5,
            CouldntResolveHost = 6,
            CouldntConnect = 7,
            WeirdServerReply = 8,
            RemoteAccessDenied = 9,
            FtpAcceptFailed = 10,
            FtpWeirdPassReply = 11,
            FtpAcceptTimeout = 12,
            FtpWeirdPasvReply = 13,
            FtpWeird227Format = 14,
            FtpCantGetHost = 15,
            Http2 = 16,
            FtpCouldntSetType = 17,
            PartialFile = 18,
            FtpCouldntRetrFile = 19,
            QuoteError = 21,
            HttpReturnedError = 22,
            WriteError = 23,
            UploadFailed = 25,
            ReadError = 26,
            OutOfMemory = 27,
            OperationTimedout = 28,
            FtpPortFailed = 30,
            FtpCouldntUseRest = 31,
            RangeError = 33,
            HttpPostError = 34,
            SslConnectError = 35,
            BadDownloadResume = 36,
            FileCouldntReadFile = 37,
            LdapCannotBind = 38,
            LdapSearchFailed = 39,
            FunctionNotFound = 41,
            AbortedByCallback = 42,
            BadFunctionArgument = 43,
            InterfaceFailed = 45,
            TooManyRedirects = 47,
            UnknownOption = 48,
            SetoptOptionSyntax = 49,
            GotNothing = 52,
            SslEngineNotfound = 53,
            SslEngineSetfailed = 54,
            SendError = 55,
            RecvError = 56,
            SslCertproblem = 58,
            SslCipher = 59,
            PeerFailedVerification = 60,
            BadContentEncoding = 61,
            LdapInvalidUrl = 62,
            FilesizeExceeded = 63,
            UseSslFailed = 64,
            SendFailRewind = 65,
            SslEngineInitfailed = 66,
            LoginDenied = 67,
            TftpNotfound = 68,
            TftpPerm = 69,
            RemoteDiskFull = 70,
            TftpIllegal = 71,
            TftpUnknownid = 72,
            RemoteFileExists = 73,
            TftpNosuchuser = 74,
            ConvFailed = 75,
            ConvReqd = 76,
            SslCacertBadfile = 77,
            RemoteFileNotFound = 78,
            Ssh = 79,
            SslShutdownFailed = 80,
            Again = 81,
            SslCrlBadfile = 82,
            SslIssuerError = 83,
            FtpPretFailed = 84,
            RtspCseqError = 85,
            RtspSessionError = 86,
            FtpBadFileList = 87,
            ChunkFailed = 88,
            NoConnectionAvailable = 89,
            SslPinnedpubkeynotmatch = 90,
            SslInvalidcertstatus = 91,
            Http2Stream = 92,
            RecursiveApiCall = 93,
            AuthError = 94,
            Http3 = 95,
            QuicConnectError = 96,
            SslClientcert = 98
        }

        [UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
        public delegate uint WriteCallback(IntPtr ptr, uint size, uint nmemb, IntPtr userData);
        //size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private UInt32 curl_global_init(UInt32 flags);
        //CURLcode curl_global_init(long flags);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private void curl_global_cleanup();
        //void curl_global_cleanup(void);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private IntPtr curl_easy_init();
        //CURL *curl_easy_init();

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private void curl_easy_cleanup(IntPtr handle);
        //void curl_easy_cleanup(CURL *handle);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "curl_easy_setopt")]
        extern static private UInt32 curl_easy_setopt_long(IntPtr handle, UInt32 option, UInt32 param);
        //CURLcode curl_easy_setopt(CURL *handle, CURLoption option, parameter);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "curl_easy_setopt")]
        extern static private UInt32 curl_easy_setopt_string(IntPtr handle, UInt32 option, string param);
        //CURLcode curl_easy_setopt(CURL *handle, CURLoption option, parameter);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "curl_easy_setopt")]
        extern static private UInt32 curl_easy_setopt_write_callback(IntPtr handle, UInt32 option, WriteCallback param);
        //CURLcode curl_easy_setopt(CURL *handle, CURLoption option, parameter);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl, EntryPoint = "curl_easy_setopt")]
        extern static private UInt32 curl_easy_setopt_blob(IntPtr handle, UInt32 option, ref Blob param);
        //CURLcode curl_easy_setopt(CURL *handle, CURLoption option, parameter);

        [DllImport(_dllPath, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        extern static private UInt32 curl_easy_perform(IntPtr handle);
        //CURLcode curl_easy_perform(CURL *easy_handle);

        static private IntPtr _handle;

        public enum LongOption
        {
            SSL_VerifyHost = 81,
            SSL_VerifyPeer = 64
        }

        public enum StringOption
        {
            URL = 10002
        }

        public enum StringBlobOption
        {
            CertificateAuthorityInfo = 40309,
            IssuerCertificate = 40295
        }

        static public void Init()
        {
            var ec = (ErrorCode)curl_global_init((uint)GlobalInitFlags.All);
            if(ec != ErrorCode.Ok)
            {
                throw new Exception("curl_global_init failed: " + ec.ToString());
            }
            _handle = curl_easy_init();
            if(_handle == IntPtr.Zero)
            {
                throw new Exception("curl_easy_init failed");
            }
        }

        static public void ShutDown()
        {
            curl_easy_cleanup(_handle);
            curl_global_cleanup();
        }

        // value is of C type "long"
        static public void SetLongOption(LongOption option, uint value)
        {
            var ec = (ErrorCode)curl_easy_setopt_long(_handle, (uint)option, value);
            if(ec != ErrorCode.Ok)
            {
                var msg = string.Format("curl_easy_setopt failed to set {0}: {1}", option, ec);
                throw new Exception(msg);
            }
        }

        // value is of C type "char*"
        static public void SetStringOption(StringOption option, string value)
        {
            var ec = (ErrorCode)curl_easy_setopt_string(_handle, (uint)option, value);
            if(ec != ErrorCode.Ok)
            {
                var msg = string.Format("curl_easy_setopt failed to set {0}: {1}", option, ec);
                throw new Exception(msg);
            }
        }

        static public void SetWriteCallback(WriteCallback value)
        {
            var ec = (ErrorCode)curl_easy_setopt_write_callback(_handle, 20011, value);
            if(ec != ErrorCode.Ok)
            {
                var msg = string.Format("curl_easy_setopt failed to set WriteCallback: {0}", ec);
                throw new Exception(msg);
            }
        }

        static public void SetStringBlobOption(StringBlobOption option, string value)
        {
            var blob = new Blob();
            blob.Data = Marshal.StringToHGlobalAnsi(value);
            blob.Length = (UIntPtr)value.Length;
            blob.Flags = 1;
            blob.Padding = 0;
            var ec = (ErrorCode)curl_easy_setopt_blob(_handle, (uint)option, ref blob);
            Marshal.FreeHGlobal(blob.Data);
            if(ec != ErrorCode.Ok)
            {
                var msg = string.Format("curl_easy_setopt failed to set {0}: {1}", option, ec);
                throw new Exception(msg);
            }
        }

        static public void Perform()
        {
            var ec = (ErrorCode)curl_easy_perform(_handle);
            if(ec != ErrorCode.Ok)
            {
                var msg = string.Format("curl_easy_perform failed: {0}", ec);
                throw new Exception(msg);
            }
        }
    }

    public class CURLStringReceiver
    {
        public string Data
        {
            get { return _builder.ToString(); }
        }

        private readonly StringBuilder _builder = new StringBuilder();

        public uint CurlWriteCallback(IntPtr ptr, uint size, uint nmemb, IntPtr userData)
        {
            uint byteCount = size * nmemb;
            _builder.Append(Marshal.PtrToStringAnsi(ptr, (int)byteCount));
            return byteCount;
        }
    }

    public class CURLDataReceiver
    {
        public List<byte> Data
        {
            get;
            private set;
        }

        public CURLDataReceiver()
        {
            Data = new List<byte>();
        }

        public uint CurlWriteCallback(IntPtr ptr, uint size, uint nmemb, IntPtr userData)
        {
            uint byteCount = size * nmemb;
            var buffer = new byte[(int)byteCount];
            Marshal.Copy(ptr, buffer, 0, buffer.Length);
            Data.AddRange(buffer);
            return byteCount;
        }
    }

    public class CURLFileReceiver : IDisposable
    {
        private FileStream _file;

        public CURLFileReceiver(string filePath)
        {
            _file = File.Create(filePath);
        }

        public uint CurlWriteCallback(IntPtr ptr, uint size, uint nmemb, IntPtr userData)
        {
            uint byteCount = size * nmemb;
            var buffer = new byte[(int)byteCount];
            Marshal.Copy(ptr, buffer, 0, buffer.Length);
            _file.Write(buffer, 0, buffer.Length);
            return byteCount;
        }

        public void Dispose()
        {
            _file.Close();
            _file.Dispose();
        }
    }
}
