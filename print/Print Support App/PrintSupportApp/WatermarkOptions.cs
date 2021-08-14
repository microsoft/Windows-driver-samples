namespace PrintSupportApp
{
    // Convenient container to store the user's currently specified watermark options
    public class WatermarkOptions
    {
        public string Text = "";
        public int TextFontSize;
        public double TextXOffset;
        public double TextYOffset;

        public bool LogoEnabled;
        public string LogoFilePath = "";
        public double LogoDpiX;
        public double LogoDpiY;
        public int LogoWidth;
        public int LogoHeight;
    }
}
