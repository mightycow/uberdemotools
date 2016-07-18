using System;
using System.IO;
using System.Text.RegularExpressions;


namespace Uber.Builder
{
    public static class VersionParser
    {
        public static bool GetVersionFromDLLHeader(SharedData data, string filePath)
        {
            var found = 0;
            var lines = File.ReadAllLines(filePath);
            foreach(var line in lines)
            {
                var tokens = line.Split(new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                if(tokens.Length != 3)
                {
                    continue;
                }

                if(tokens[0] != "#define")
                {
                    continue;
                }

                int value = 0;
                if(!int.TryParse(tokens[2], out value) ||
                    value < 0)
                {
                    continue;
                }

                if(tokens[1] == "UDT_VERSION_MAJOR")
                {
                    found |= 1;
                    data.DLLVersion.Major = value;
                }
                else if(tokens[1] == "UDT_VERSION_MINOR")
                {
                    found |= 2;
                    data.DLLVersion.Minor = value;
                }
                else if(tokens[1] == "UDT_VERSION_REVISION")
                {
                    found |= 4;
                    data.DLLVersion.Revision = value;
                }
                else
                {
                    continue;
                }

                if(found == 7)
                {
                    return true;
                }
            }

            return false;
        }

        private static Regex ViewerVersionStringRegEx = new Regex("ViewerVersionString = \"(\\d+)\\.(\\d+)\\.(\\d+)\"", RegexOptions.Compiled);

        public static bool GetVersionFromViewerSource(SharedData data, string filePath)
        {
            var lines = File.ReadAllLines(filePath);
            foreach(var line in lines)
            {
                var match = ViewerVersionStringRegEx.Match(line);
                if(match.Success)
                {
                    var major = 0;
                    var minor = 0;
                    var rev = 0;
                    if(int.TryParse(match.Groups[1].Value, out major) &&
                        int.TryParse(match.Groups[2].Value, out minor) &&
                        int.TryParse(match.Groups[3].Value, out rev))
                    {
                        data.ViewerVersion.Major = major;
                        data.ViewerVersion.Minor = minor;
                        data.ViewerVersion.Revision = rev;
                        return true;
                    }
                    break;
                }
            }

            return false;
        }

        private static Regex GuiVersionStringRegEx = new Regex("GuiVersion = \"(\\d+)\\.(\\d+)\\.(\\d+)\"", RegexOptions.Compiled);
        private static Regex MinimumDllVersionMajorRegEx = new Regex("MinimumDllVersionMajor = (\\d+)", RegexOptions.Compiled);
        private static Regex MinimumDllVersionMinorRegEx = new Regex("MinimumDllVersionMinor = (\\d+)", RegexOptions.Compiled);
        private static Regex MinimumDllVersionRevisionRegEx = new Regex("MinimumDllVersionRevision = (\\d+)", RegexOptions.Compiled);

        public static bool GetVersionsFromCSAppSource(SharedData data, string filePath)
        {
            var found = 0;
            var lines = File.ReadAllLines(filePath);
            foreach(var line in lines)
            {
                if(found == 15)
                {
                    return true;
                }

                var match = GuiVersionStringRegEx.Match(line);
                if(match.Success)
                {
                    var major = 0;
                    var minor = 0;
                    var rev = 0;
                    if(int.TryParse(match.Groups[1].Value, out major) &&
                        int.TryParse(match.Groups[2].Value, out minor) &&
                        int.TryParse(match.Groups[3].Value, out rev))
                    {
                        found |= 1;
                        data.GUIVersion.Major = major;
                        data.GUIVersion.Minor = minor;
                        data.GUIVersion.Revision = rev;
                    }
                    continue;
                }

                var val = 0;
                match = MinimumDllVersionMajorRegEx.Match(line);
                if(match.Success)
                {
                    if(int.TryParse(match.Groups[1].Value, out val))
                    {
                        found |= 2;
                        data.DLLVersionRequiredByGUI.Major = val;
                    }
                    continue;
                }

                match = MinimumDllVersionMinorRegEx.Match(line);
                if(match.Success)
                {
                    if(int.TryParse(match.Groups[1].Value, out val))
                    {
                        found |= 4;
                        data.DLLVersionRequiredByGUI.Minor = val;
                    }
                    continue;
                }

                match = MinimumDllVersionRevisionRegEx.Match(line);
                if(match.Success)
                {
                    if(int.TryParse(match.Groups[1].Value, out val))
                    {
                        found |= 8;
                        data.DLLVersionRequiredByGUI.Revision = val;
                    }
                    continue;
                }
            }

            return found == 15;
        }

        private static Regex VersionRegEx = new Regex("(\\d+)\\.(\\d+)\\.(\\d+)", RegexOptions.Compiled);

        private static bool GetVersionAtStartOfFile(Version version, string filePath)
        {
            var match = VersionRegEx.Match(File.ReadAllText(filePath));
            if(match.Success && match.Index == 0)
            {
                var major = 0;
                var minor = 0;
                var rev = 0;
                if(int.TryParse(match.Groups[1].Value, out major) &&
                    int.TryParse(match.Groups[2].Value, out minor) &&
                    int.TryParse(match.Groups[3].Value, out rev))
                {
                    version.Major = major;
                    version.Minor = minor;
                    version.Revision = rev;
                    return true;
                }
            }

            return false;
        }

        public static bool GetVersionFromGUIChangeLog(SharedData data, string filePath)
        {
            return GetVersionAtStartOfFile(data.ChangeLogGUIVersion, filePath);
        }

        public static bool GetVersionFromDLLChangeLog(SharedData data, string filePath)
        {
            return GetVersionAtStartOfFile(data.ChangeLogDLLVersion, filePath);
        }

        public static bool GetVersionFromViewerChangeLog(SharedData data, string filePath)
        {
            return GetVersionAtStartOfFile(data.ChangeLogViewerVersion, filePath);
        }
    }
}