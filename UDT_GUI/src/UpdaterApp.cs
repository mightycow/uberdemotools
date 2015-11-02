using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Net;
using System.IO;
using System.IO.Compression;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using System.Threading;


namespace Uber.DemoTools.Updater
{
    public class Updater
    {
        public class UpdaterArg
        {
            public string DllVersion;
            public string GuiVersion;
            public string ArchName;
            public int ParentProcessId;
        }

        static public void UpdateUDT(UpdaterArg arg)
        {
            var updater = new Updater(arg);
            updater.Update();
        }

        public Updater(UpdaterArg arg)
        {
            _curDllVersion = arg.DllVersion;
            _curGuiVersion = arg.GuiVersion;
            _archName = arg.ArchName;
            _parentProcessId = arg.ParentProcessId;
        }

        private void Update()
        {
            _webClient = new WebClient();
            GetLatestVersionNumbersAndBinariesUrl();
            if(!IsNewVersionHigher())
            {
                MessageBox.Show("You are already running the latest version.", "UDT Updater", MessageBoxButtons.OK, MessageBoxIcon.Information);
                _webClient.Dispose();
                return;
            }

            var result = MessageBox.Show("A new version is available. Close UDT and update now?", "UDT Updater", MessageBoxButtons.OKCancel, MessageBoxIcon.Information);
            if(result != DialogResult.OK)
            {
                _webClient.Dispose();
                return;
            }

            try
            {
                var parentProcess = Process.GetProcessById(_parentProcessId);
                parentProcess.CloseMainWindow();
                parentProcess.WaitForExit();
            }
            catch(Exception)
            {
            }

            DownloadAndReadConfig();
            if(_filesToAdd.Count == 0)
            {
                MessageBox.Show("Failed to access update information.", "UDT Updater", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            DownloadAndExtractNewTempFiles();
            ApplyPatch();

            Process.Start("UDT_GUI.exe");

            _webClient.Dispose();
        }

        private bool GetLatestVersionNumbersAndBinariesUrl()
        {
            var htmlData = _webClient.DownloadString("http://giant.pourri.ch/snif.php?path=UDT/");
            var regEx = new Regex("udt_gui_(.+)_dll_(.+)_" + _archName + ".zip");
            var match = regEx.Match(htmlData);
            if(!match.Success)
            {
                return false;
            }

            _downloadUrl = "http://giant.pourri.ch/UDT/" + match.Groups[0].Value;
            _newGuiVersion = match.Groups[1].Value;
            _newDllVersion = match.Groups[2].Value;

            return true;
        }

        private bool DownloadAndReadConfig()
        {
            var config = _webClient.DownloadString("http://giant.pourri.ch/UDT/updater/updater.cfg");
            using(StringReader sr = new StringReader(config))
            {
                string line;
                while((line = sr.ReadLine()) != null)
                {
                    if(!string.IsNullOrWhiteSpace(line))
                    {
                        ProcessConfigEntry(line);
                    }
                }
            }

            return true;
        }

        private void ProcessConfigEntry(string line)
        {
            if(line.StartsWith("add "))
            {
                _filesToAdd.Add(line.Substring(4));
            }
            else if(line.StartsWith("rem "))
            {
                _filesToRemove.Add(line.Substring(4));
            }
        }

        private bool DownloadAndExtractNewTempFiles()
        {
            var filesNotFound = new List<string>();
            var data = _webClient.DownloadData(_downloadUrl);
            using(var memoryStream = new MemoryStream(data))
            {
                using(var archive = ZipStorer.Open(memoryStream, FileAccess.Read))
                {
                    var centralDir = archive.ReadCentralDir();
                    foreach(var fileName in _filesToAdd)
                    {
                        bool found = false;
                        foreach(var entry in centralDir)
                        {
                            if(entry.FilenameInZip == fileName)
                            {
                                archive.ExtractFile(entry, fileName);
                                break;
                            }
                        }

                        if(!found)
                        {
                            filesNotFound.Add(fileName);
                        }
                    }
                }
            }

            foreach(var fileName in filesNotFound)
            {
                _filesToAdd.Remove(fileName);
            }

            return true;
        }

        private void ApplyPatch()
        {
            foreach(var fileName in _filesToAdd)
            {
                File.Delete(fileName + UpdaterHelper.OldFileExtension);
            }

            foreach(var fileName in _filesToAdd)
            {
                if(File.Exists(fileName))
                {
                    File.Move(fileName, fileName + UpdaterHelper.OldFileExtension);
                }
            }

            foreach(var fileName in _filesToAdd)
            {
                File.Move(fileName + UpdaterHelper.NewFileExtension, fileName);
            }

            foreach(var fileName in _filesToAdd)
            {
                File.Delete(fileName + UpdaterHelper.OldFileExtension);
            }

            foreach(var fileName in _filesToRemove)
            {
                File.Delete(fileName);
            }
        }

        private static int GetVersionNumberFromString(string v)
        {
            var match = _versionRegEx.Match(v);
            if(!match.Success)
            {
                return 0;
            }

            var n1 = int.Parse(match.Groups[1].Value);
            var n2 = int.Parse(match.Groups[2].Value);
            var n3 = int.Parse(match.Groups[3].Value);
            var n4 = match.Groups[4].Value.Length == 1 ? (int)('b' - match.Groups[4].Value[0]) : 0;
            var version = n4 + n3 * (100) + n2 * (100 * 100) + n1 * (100 * 100 * 100);

            return version;
        }

        private static int CompareStringVersions(string a, string b)
        {
            if(a == b)
            {
                return 0;
            }

            return GetVersionNumberFromString(a) - GetVersionNumberFromString(b);
        }

        private bool IsNewVersionHigher()
        {
            return CompareStringVersions(_newDllVersion, _curDllVersion) > 0 ||
                CompareStringVersions(_newGuiVersion, _curGuiVersion) > 0;
        }

        private static readonly Regex _versionRegEx = new Regex("(\\d+)\\.(\\d+)\\.(\\d+)([a-z])?", RegexOptions.Compiled);
        private readonly List<string> _filesToAdd = new List<string>();
        private readonly List<string> _filesToRemove = new List<string>();
        private WebClient _webClient;
        private string _curDllVersion;
        private string _curGuiVersion;
        private string _archName;
        private string _newDllVersion;
        private string _newGuiVersion;
        private string _downloadUrl;
        private int _parentProcessId;
    }


    static class MainClass
    {
        static void OnArgumentError()
        {
            MessageBox.Show("Please run the updater from the UDT interface.", "UDT Updater", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        static void RealMain(string[] arguments)
        {
            if(arguments.Length != 4)
            {
                OnArgumentError();
                return;
            }

            foreach(var arg in arguments)
            {
                if(string.IsNullOrWhiteSpace(arg))
                {
                    OnArgumentError();
                    return;
                }
            }

            if(arguments[2] != "x86" && arguments[2] != "x64")
            {
                OnArgumentError();
                return;
            }

            var processId = 0;
            if(!int.TryParse(arguments[3], out processId))
            {
                OnArgumentError();
                return;
            }

            var updaterArg = new Updater.UpdaterArg();
            updaterArg.DllVersion = arguments[0];
            updaterArg.GuiVersion = arguments[1];
            updaterArg.ArchName = arguments[2];
            updaterArg.ParentProcessId = processId;
            Updater.UpdateUDT(updaterArg);
        }

        public static void Main(string[] arguments)
        {
            var mutex = UpdaterHelper.TryOpenNamedMutex();
            if(mutex != null)
            {
                // Updater already running.
                return;
            }

            mutex = UpdaterHelper.CreateNamedMutex();
            if(mutex == null)
            {
                MessageBox.Show("Mutex creation failed. :-(", "UDT Updater", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            try
            {
                RealMain(arguments);
            }
            catch(Exception e)
            {
                MessageBox.Show("Exception caught: " + e.Message, "UDT Updater", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                mutex.Close();
                mutex.Dispose();
            }
        }
    }
}