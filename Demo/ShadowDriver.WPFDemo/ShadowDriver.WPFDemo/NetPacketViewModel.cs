namespace ShadowDriver.WPFDemo
{
    public class NetPacketViewModel
    {
        public string Content { set; get; } = string.Empty;

        public override string ToString()
        {
            return Content;
        }
    }
}