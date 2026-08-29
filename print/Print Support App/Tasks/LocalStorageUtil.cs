using System;
using System.Collections.Generic;
using System.Linq;
using Windows.Devices.Printers;
using Windows.Security.Cryptography;
using Windows.Security.Cryptography.Core;
using Windows.Storage;

namespace Tasks
{
    public sealed class LocalStorageUtil
    {
        /// <summary>
        /// Stores a dictionary of IPP custom attributes to local storage. These can then be retrieved with
        /// GetCustomIppAttributesFromLocalStorage()
        /// </summary>
        /// <param name="attributes">Dictionary of attributes to save. Only "keyword" attributes are supported</param>
        public static void SaveCustomIppAttributesToLocalStorage(IDictionary<string, IppAttributeValue> attributes)
        {
            foreach (var attributeValue in attributes.Values)
            {
                if (attributeValue.Kind != IppAttributeValueKind.Keyword)
                {
                    throw new NotImplementedException($"Cannot store IppAttributeValue of type {attributeValue.Kind}");
                }
            }

            ApplicationData.Current.LocalSettings.Values[IppAttributeValueKeysCanonicalName] = string.Join(",", attributes.Keys);

            foreach (KeyValuePair<string, IppAttributeValue> attribute in attributes)
            {
                string keyName = attribute.Key;
                IList<string> keywordArray = attribute.Value.GetKeywordArray();

                ApplicationData.Current.LocalSettings.Values[$"{IppAttributeValueKeysCanonicalName}-{keyName}"] = string.Join(",", keywordArray);
            }
        }

        /// <summary>
        /// Retrieve custom IPP attributes from local storage.
        /// </summary>
        /// <returns>Dictionary of custom attributes, ready for use</returns>
        public static IDictionary<string, IppAttributeValue> GetCustomIppAttributesFromLocalStorage()
        {
            GetValueFromLocalStorage(ApplicationData.Current.LocalSettings, IppAttributeValueKeysCanonicalName, out string storedKeyString);
            IEnumerable<string> storedKeys = storedKeyString?.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries) ?? new string[] { };

            return storedKeys.ToDictionary(keyName => keyName, keyName =>
            {
                GetValueFromLocalStorage(ApplicationData.Current.LocalSettings, $"{IppAttributeValueKeysCanonicalName}-{keyName}", out string storedValueString);
                IEnumerable<string> storedKeywords = storedValueString?.Split(",") ?? new string[] { };

                return IppAttributeValue.CreateKeywordArray(storedKeywords);
            });
        }

        public static void SaveAndEncryptJobPassword(string password, string encryptionMethod)
        {
            var buffer = CryptographicBuffer.ConvertStringToBinary(password, BinaryStringEncoding.Utf8);
            HashAlgorithmProvider algoProvider = null;
            if (encryptionMethod == "none")
            {
                // do nothing.
            }
            else if (encryptionMethod == "sha2-256")
            {
                algoProvider = HashAlgorithmProvider.OpenAlgorithm("SHA256");
            }

            if (algoProvider != null)
            {
                var hashedBuffer = algoProvider.HashData(buffer);
                ApplicationData.Current.LocalSettings.Values[JobPasswordCanonicalName] = CryptographicBuffer.EncodeToBase64String(hashedBuffer);
            }
            else
            {
                ApplicationData.Current.LocalSettings.Values[JobPasswordCanonicalName] = CryptographicBuffer.EncodeToBase64String(buffer);
            }

            ApplicationData.Current.LocalSettings.Values[JobPasswordEncryptionMethodCanonicalName] = encryptionMethod;
        }

        public static Windows.Storage.Streams.IBuffer GetEncryptedJobPassword()
        {
            var store = ApplicationData.Current.LocalSettings.Values;
            if (store.ContainsKey(JobPasswordCanonicalName))
            {
                return CryptographicBuffer.DecodeFromBase64String(store[JobPasswordCanonicalName].ToString());
            }

            return null;
        }

        public static string GetJobPasswordEncryptionMethod()
        {
            var store = ApplicationData.Current.LocalSettings.Values;
            if (store.ContainsKey(JobPasswordEncryptionMethodCanonicalName))
            {
                return store[JobPasswordEncryptionMethodCanonicalName].ToString();
            }

            return null;
        }

        public static void ClearJobPassword()
        {
            var store = ApplicationData.Current.LocalSettings.Values;
            if (store.ContainsKey(JobPasswordCanonicalName))
            {
                store.Remove(JobPasswordCanonicalName);
            }

            if (store.ContainsKey(JobPasswordEncryptionMethodCanonicalName))
            {
                store.Remove(JobPasswordEncryptionMethodCanonicalName);
            }
        }

        /// <summary>
        /// Reset the watermark text & image properties from local storage
        /// </summary>
        public static void ResetWatermarkTextAndImage()
        {
            SaveWatermarkTextPropertiesToLocalStorage("", 0, 0.0, 0.0);
            ApplicationData.Current.LocalSettings.Values[ImageUsingCanonicalName] = false;
        }


        /// <summary>
        /// Set the watermark text properties from local storage
        /// </summary>
        /// <param name="WatermarkText">Text of Watermark"</param>
        /// <param name="fontSize">Size of Font </param>
        /// <param name="xPos">[0,1] range of x position on Document</param>
        /// <param name="yPos">[0,1] range of y posiiton on Document</param>
        public static void SaveWatermarkTextPropertiesToLocalStorage(string Watermark, int fontSize, double xPos, double yPos)
        {
            ApplicationData.Current.LocalSettings.Values[WatermartTextCanonicalName] = Watermark;
            ApplicationData.Current.LocalSettings.Values[WatermarkTextSizeCanonicalName] = fontSize;
            ApplicationData.Current.LocalSettings.Values[WatermarkXposCanonicalName] = xPos;
            ApplicationData.Current.LocalSettings.Values[WatermarkYposCanonicalName] = yPos;
        }

        /// <summary>
        /// Save the image properties to local storage
        /// If it were required to be able to distinguish between sessions, you could prefix
        /// with something unique to that session, in addition to the "canonical name"
        /// </summary>
        /// <param name="imageFile">Image file</param>
        /// <param name="dpiX">Image dots per inch X</param>
        /// <param name="dpiY">Image dots per inch Y</param>
        /// <param name="imageWidth">Image width in pixels</param>
        /// <param name="imageHeight">Image height in pixels</param>
        public static void SaveImagePropertiesToLocalStorage(string imageFile, double dpiX, double dpiY, int imageWidth, int imageHeight)
        {
            ApplicationData.Current.LocalSettings.Values[ImagePathCanonicalName] = imageFile;
            ApplicationData.Current.LocalSettings.Values[ImageDpiXCanonicalName] = dpiX;
            ApplicationData.Current.LocalSettings.Values[ImageDpiYCanonicalName] = dpiY;
            ApplicationData.Current.LocalSettings.Values[ImageWidthCanonicalName] = imageWidth;
            ApplicationData.Current.LocalSettings.Values[ImageHeightCanonicalName] = imageHeight;
            ApplicationData.Current.LocalSettings.Values[ImageUsingCanonicalName] = true;
        }



        /// <summary>
        /// Get the image properties from local storage
        /// </summary>
        /// <param name="imageFile">Image file, e.g. "Assets\\MyImage.jpg"</param>
        /// <param name="dpiX">Image dots per inch X</param>
        /// <param name="dpiY">Image dots per inch Y</param>
        /// <param name="width">Image width in pixels</param>
        /// <param name="height">Image height in pixels</param>
        public static bool GetImagePropertiesFromLocalStorage(out string imageFile, out double dpiX, out double dpiY, out int width, out int height)
        {
            // Look for the watermark text in local storage
            var localSettings = ApplicationData.Current.LocalSettings;
            bool retval = true;
            if (!GetValueFromLocalStorage(localSettings, ImagePathCanonicalName, out imageFile))
            {
                retval = false;
            }
            if (!GetValueFromLocalStorage(localSettings, ImageDpiXCanonicalName, out dpiX))
            {
                retval = false;
            }
            if (!GetValueFromLocalStorage(localSettings, ImageHeightCanonicalName, out height))
            {
                retval = false;
            }
            if (!GetValueFromLocalStorage(localSettings, ImageDpiYCanonicalName, out dpiY))
            {
                retval = false;
            }
            if (!GetValueFromLocalStorage(localSettings, ImageWidthCanonicalName, out width))
            {
                retval = false;
            }

            return retval && GetValueFromLocalStorage(localSettings, ImageUsingCanonicalName, out bool isUsingWatermarkPicture) && isUsingWatermarkPicture;
        }

        /// <summary>
        /// Get the watermark text properties from local storage
        /// </summary>
        /// <param name="WatermarkText">Text of Watermark"</param>
        /// <param name="fontSize">Size of Font </param>
        /// <param name="xPos">[0,1] range of x position on Document</param>
        /// <param name="yPos">[0,1] range of y posiiton on Document</param>
        public static bool GetWatermarkTextPropertiesFromLocalStorage(out string WatermarkText, out int fontSize, out double xPos, out double yPos)
        {
            var localSettings = ApplicationData.Current.LocalSettings;
            bool retval = true;
            if (!GetValueFromLocalStorage(localSettings, WatermartTextCanonicalName, out WatermarkText))
            {
                retval = false;
            }
            if (!GetValueFromLocalStorage(localSettings, WatermarkTextSizeCanonicalName, out fontSize))
            {
                retval = false;
            }
            if (!GetValueFromLocalStorage(localSettings, WatermarkXposCanonicalName, out xPos))
            {
                retval = false;
            }
            if (!GetValueFromLocalStorage(localSettings, WatermarkYposCanonicalName, out yPos))
            {
                retval = false;
            }

            return retval && !string.IsNullOrEmpty(WatermarkText);
        }

        /// <summary>
        /// Retrieve required property from local storage
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="localSettings">Windows.Storage.ApplicationDataContainer in app local storage</param>
        /// <param name="propertyName">Name of proprty to retrieve</param>
        /// <param name="requiredValue">Property value returned</param>
        /// <returns>True if property was found, else false if not</returns>
        private static bool GetValueFromLocalStorage<T>(ApplicationDataContainer localSettings, string propertyName, out T requiredValue)
        {
            bool foundProperty = false;
            object o = localSettings.Values[propertyName];
            if (o != null)
            {
                requiredValue = (T)o;
                foundProperty = true;
            }
            else
            {
                requiredValue = default(T);
            }

            return foundProperty;
        }

        private const string WatermarkTextSizeCanonicalName = "WatermarkTextSize";
        private const string WatermartTextCanonicalName = "WatermarkText";
        private const string WatermarkXposCanonicalName = "WatermarkXpos";
        private const string WatermarkYposCanonicalName = "WatermarkYpos";

        private const string ImagePathCanonicalName = "WatermarkImage";
        private const string ImageDpiXCanonicalName = "WatermarkImageDpiX";
        private const string ImageDpiYCanonicalName = "WatermarkImageDpiY";
        private const string ImageWidthCanonicalName = "WatermarkImageWidth";
        private const string ImageHeightCanonicalName = "WatermarkImageHeight";
        private const string ImageUsingCanonicalName = "WatermarkImageSet";

        private const string IppAttributeValueKeysCanonicalName = "IppAttributeValueKeys";

        private const string JobPasswordCanonicalName = "JobPassword";
        private const string JobPasswordEncryptionMethodCanonicalName = "JobPasswordEcryptionMethod";
    }
}
