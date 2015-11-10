using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Net;
using System.IO;
using System.IO.Compression;
using System.Runtime.InteropServices;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;


namespace Uber.DemoTools.Updater
{
    public static class VersionInfo
    {
        public static readonly string Version = "0.1.0";
    }

    public static class FileOps
    {
        public static bool TryDelete(string filePath)
        {
            try
            {
                if(File.Exists(filePath))
                {
                    File.Delete(filePath);
                }
                return true;
            }
            catch(Exception)
            {
            }

            return false;
        }

        public static bool TryMove(string sourceFilePath, string destFilePath)
        {
            try
            {
                if(File.Exists(sourceFilePath))
                {
                    File.Move(sourceFilePath, destFilePath);
                    return true;
                }
            }
            catch(Exception)
            {
            }

            return false;
        }
    }

    public static class Tokenizer
    {
        public static List<string> Tokenize(string line)
        {
            var tokens = new List<string>();
            if(string.IsNullOrWhiteSpace(line))
            {
                return tokens;
            }

            if(line.StartsWith("//"))
            {
                return tokens;
            }

            var i = 0;
            while(i < line.Length)
            {
                var c = line[i];
                if(char.IsWhiteSpace(c))
                {
                    ++i;
                    continue;
                }
                else if(c == '"')
                {
                    var endQuoteIdx = line.IndexOf('"', i + 1);
                    if(endQuoteIdx < 0)
                    {
                        break;
                    }
                    tokens.Add(line.Substring(i + 1, endQuoteIdx - i - 1));
                    i = endQuoteIdx + 1;
                }
                else
                {
                    var startTokenIdx = i++;
                    while(i < line.Length)
                    {
                        if(char.IsWhiteSpace(line[i++]))
                        {
                            break;
                        }
                    }
                    var endOffset = i == line.Length ? 0 : 1;
                    tokens.Add(line.Substring(startTokenIdx, i - startTokenIdx - endOffset));
                }
            }

            return tokens;
        }
    }

    public static class ConfigParser
    {
        public class ConfigCommand
        {
            public string Command = "";
            public readonly List<string> Arguments = new List<string>();
            public readonly List<string> Options = new List<string>();
        }

        public static ConfigCommand ParseCommand(string line)
        {
            var tokens = Tokenizer.Tokenize(line);
            if(tokens.Count == 0)
            {
                return null;
            }

            var command = new ConfigCommand();
            var commandIdx = 0;
            foreach(var token in tokens)
            {
                if(token.StartsWith("[") && token.EndsWith("]"))
                {
                    ++commandIdx;
                    command.Options.Add(token.Substring(1, token.Length - 2));
                }
                else
                {
                    break;
                }
            }

            if(commandIdx >= tokens.Count)
            {
                return null;
            }

            command.Command = tokens[commandIdx];
            for(var i = commandIdx + 1; i < tokens.Count; ++i)
            {
                command.Arguments.Add(tokens[i]);
            }

            return command;
        }
    }

    public class Updater
    {
        public class UpdaterArg
        {
            public string DllVersion;
            public string GuiVersion;
            public string ArchName;
            public int ParentProcessId;
            public bool NoMessageBoxIfCurrent;
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
            _noMessageBoxIfCurrent = arg.NoMessageBoxIfCurrent;
        }

        private void Update()
        {
            using(var webClient = new WebClient())
            {
                _webClient = webClient;

                GetLatestVersionNumbersAndBinariesUrl();
                if(!IsNewVersionHigher())
                {
                    if(!_noMessageBoxIfCurrent)
                    {
                        MessageBox.Show("You are already running the latest version.", "UDT Updater", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                    return;
                }

                var result = MessageBox.Show("A new version is available. Close UDT and update now?", "UDT Updater", MessageBoxButtons.OKCancel, MessageBoxIcon.Information);
                if(result != DialogResult.OK)
                {
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
                if(!_fileOps.Exists(o => o.Operation == FileOperationType.ExtractFile && o.AllowedToFail == false))
                {
                    MessageBox.Show("Failed to retrieve valid update information.", "UDT Updater", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }

                var patchSuccessful = false;
                try
                {
                    if(DownloadAndExtractNewTempFiles() && ApplyPatch())
                    {
                        patchSuccessful = true;
                    }
                }
                catch(Exception)
                {
                }
                finally
                {
                    if(!patchSuccessful)
                    {
                        TryUndoPatch();
                    }
                }

                if(!patchSuccessful)
                {
                    MessageBox.Show("Failed to patch the existing install.\nTried to roll back as well as possible.", "UDT Updater", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }

                if(patchSuccessful || (File.Exists("UDT_GUI.exe") && File.Exists("UDT.dll")))
                {
                    Process.Start("UDT_GUI.exe");
                }
            }
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
                    var command = ConfigParser.ParseCommand(line);
                    if(command != null)
                    {
                        ProcessConfigCommand(command);
                    }
                }
            }

            return true;
        }

        private void ProcessConfigCommand(ConfigParser.ConfigCommand command)
        {
            var op = new FileOperation();
            op.AllowedToFail = command.Options.Exists(o => o == "canfail");
            if(command.Command == "extract" && command.Arguments.Count == 1)
            {
                op.Operation = FileOperationType.ExtractFile;
                op.FilePath = command.Arguments[0];
                _fileOps.Add(op);
            }
            else if(command.Command == "delete" && command.Arguments.Count == 1)
            {
                op.Operation = FileOperationType.DeleteFile;
                op.FilePath = command.Arguments[0];
                _fileOps.Add(op);
            }
            else if(command.Command == "rename" && command.Arguments.Count == 2)
            {
                op.Operation = FileOperationType.RenameFile;
                op.FilePath = command.Arguments[0];
                op.FilePath2 = command.Arguments[1];
                _fileOps.Add(op);
            }
        }

        private bool DownloadAndExtractNewTempFiles()
        {
            var data = _webClient.DownloadData(_downloadUrl);
            using(var archive = ZipStorer.Open(new MemoryStream(data), FileAccess.Read))
            {
                var entries = archive.ReadCentralDir();
                var extractOps = _fileOps.FindAll(o => o.Operation == FileOperationType.ExtractFile);
                foreach(var op in extractOps)
                {
                    var entryIdx = entries.FindIndex(e => e.FilenameInZip == op.FilePath);
                    if(entryIdx < 0)
                    {
                        if(op.AllowedToFail)
                        {
                            continue;
                        }
                        else
                        {
                            return false;
                        }
                    }

                    var entry = entries[entryIdx];
                    var success = archive.ExtractFile(entry, op.FilePath + UpdaterHelper.NewFileExtension);
                    if(success)
                    {
                        AddUndoDelete(op.FilePath + UpdaterHelper.NewFileExtension);
                    }
                    else if(!success && !op.AllowedToFail)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        private bool ApplyPatch()
        {
            var extractOps = _fileOps.FindAll(o => o.Operation == FileOperationType.ExtractFile);
            var deleteOps = _fileOps.FindAll(o => o.Operation == FileOperationType.DeleteFile);
            var renameOps = _fileOps.FindAll(o => o.Operation == FileOperationType.RenameFile);

            // Delete files that may be dangling from a previously failed update attempt.
            foreach(var op in extractOps)
            {
                TryDeleteFile(op.FilePath + UpdaterHelper.OldFileExtension);
            }

            // Rename the currently valid files for backup.
            foreach(var op in extractOps)
            {
                TryMoveFileWithUndo(op.FilePath, op.FilePath + UpdaterHelper.OldFileExtension);
            }

            // Rename the freshly extracted files to be the new current ones.
            foreach(var op in extractOps)
            {
                if(!TryMoveFileWithUndo(op.FilePath + UpdaterHelper.NewFileExtension, op.FilePath) && !op.AllowedToFail)
                {
                    return false;
                }
            }

            // Execute rename operations from the updater config.
            foreach(var op in renameOps)
            {
                if(!TryMoveFileWithUndo(op.FilePath, op.FilePath2) && !op.AllowedToFail)
                {
                    return false;
                }
            }

            // Execute delete operations from the updater config.
            foreach(var op in deleteOps)
            {
                if(!TryDeleteFile(op.FilePath) && !op.AllowedToFail)
                {
                    return false;
                }
            }

            // Get rid of the backup files.
            foreach(var op in extractOps)
            {
                TryDeleteFile(op.FilePath + UpdaterHelper.OldFileExtension);
            }

            return true;
        }

        private void TryUndoPatch()
        {
            _undoFileOps.Reverse();
            foreach(var op in _undoFileOps)
            {
                try
                {
                    if(op.Operation == FileOperationType.DeleteFile)
                    {
                        File.Delete(op.FilePath);
                    }
                    else if(op.Operation == FileOperationType.RenameFile)
                    {
                        File.Move(op.FilePath, op.FilePath2);
                    }
                }
                catch(Exception)
                {
                }
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
            var n4 = match.Groups[4].Value.Length == 1 ? (int)(1 + char.ToLower(match.Groups[4].Value[0]) - 'a') : 0;
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

        private void AddUndoDelete(string filePath)
        {
            var op = new FileOperation();
            op.Operation = FileOperationType.DeleteFile;
            op.FilePath = filePath;
            _undoFileOps.Add(op);
        }

        private void AddUndoRename(string filePath, string filePath2)
        {
            var op = new FileOperation();
            op.Operation = FileOperationType.RenameFile;
            op.FilePath = filePath;
            op.FilePath2 = filePath2;
            _undoFileOps.Add(op);
        }

        private bool TryDeleteFile(string filePath)
        {
            return FileOps.TryDelete(filePath);
        }

        private bool TryMoveFileWithUndo(string filePath, string filePath2)
        {
            if(FileOps.TryMove(filePath, filePath2))
            {
                AddUndoRename(filePath, filePath2);
                return true;
            }

            return false;
        }

        public enum FileOperationType
        {
            None,
            ExtractFile,
            RenameFile,
            DeleteFile
        }

        public class FileOperation
        {
            public FileOperationType Operation = FileOperationType.None;
            public string FilePath = "";
            public string FilePath2 = "";
            public bool AllowedToFail = false;
        }

        private static readonly Regex _versionRegEx = new Regex("(\\d+)\\.(\\d+)\\.(\\d+)([a-z])?", RegexOptions.Compiled);
        private readonly List<FileOperation> _fileOps = new List<FileOperation>();
        private readonly List<FileOperation> _undoFileOps = new List<FileOperation>();
        private WebClient _webClient;
        private string _curDllVersion;
        private string _curGuiVersion;
        private string _archName;
        private string _newDllVersion;
        private string _newGuiVersion;
        private string _downloadUrl;
        private int _parentProcessId;
        private bool _noMessageBoxIfCurrent;
    }

    static class MainClass
    {
        static void OnArgumentError()
        {
            MessageBox.Show("Please run the updater from the UDT interface.", "UDT Updater", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        static void RealMain(string[] arguments)
        {
            if(arguments.Length != 5)
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
            updaterArg.NoMessageBoxIfCurrent = arguments[4] == UpdaterHelper.NoMessageBoxIfCurrentArg;
            Updater.UpdateUDT(updaterArg);
        }

        public static void Main(string[] arguments)
        {
            Mutex mutex = null;
            try
            {
                if(arguments.Length == 1 && arguments[0] == "/?")
                {
                    MessageBox.Show("UDT Updater version: " + VersionInfo.Version, "UDT Updater", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    return;
                }

                mutex = UpdaterHelper.TryOpenNamedMutex();
                if(mutex != null)
                {
                    // Updater already running.
                    return;
                }

                mutex = UpdaterHelper.TryCreateNamedMutex();
                if(mutex == null)
                {
                    MessageBox.Show("Mutex creation failed. :-(", "UDT Updater", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }

                RealMain(arguments);
            }
            catch(Exception e)
            {
                MessageBox.Show("Exception caught: " + e.Message + "\n" + e.StackTrace, "UDT Updater", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                if(mutex != null)
                {
                    mutex.Close();
                }
            }
        }
    }
}