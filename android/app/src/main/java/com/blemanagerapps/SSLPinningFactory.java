package com.blemanagerapps; // Update with your package name

import com.facebook.react.modules.network.OkHttpClientFactory;
import com.facebook.react.modules.network.OkHttpClientProvider;
import okhttp3.CertificatePinner;
import okhttp3.OkHttpClient;

public class SSLPinningFactory implements OkHttpClientFactory {
   private static String hostname = "ahm-dev.acedigitalcloudplatforms.com"; // Update with your domain

   public OkHttpClient createNewNetworkModuleClient() {

      CertificatePinner certificatePinner = new CertificatePinner.Builder()
        .add(hostname, "sha256/BiCJlyfJSWcbGTnE9sazpW5EqH9uxqUWQ1EbXs78jFY=") // Replace with your SHA256 key
        .build();

      OkHttpClient.Builder clientBuilder = OkHttpClientProvider.createClientBuilder();
      return clientBuilder.certificatePinner(certificatePinner).build();
  }
}