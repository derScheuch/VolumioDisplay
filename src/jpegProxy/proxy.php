<?php

if (isset($_GET['url'])) {
    $imageUrl = $_GET['url'];
   

    if (substr_compare($imageUrl, "http", 0, 4) === 0 ) {  
        if (substr_compare($imageUrl, "https", 0, 5) === 0 ) {
            $remote_image = file_get_contents( "http" .substr($imageUrl,5 ));
        } else {
            $remote_image = file_get_contents( $imageUrl);
        }
    } else {
        $remote_image = file_get_contents("http://192.168.178.50" . $imageUrl);
    }
    file_put_contents("/tmp/remote_image.jpg", $remote_image);
    $image = new Imagick("/tmp/remote_image.jpg");

    $image->resizeImage(64,64,Imagick::FILTER_LANCZOS,1);

    $width = $image->getImageWidth();
    $height = $image->getImageHeight();
    if ($width != 64 || $height != 64)  {
        $image->resizeImage(64,64,Imagick::FILTER_LANCZOS,1);
        $width = $image->getImageWidth();
        $height = $image->getImageHeight();
      
    }
    


    $rgbValues = [];
    $pixels = $image->exportImagePixels(0, 0, $width, $height, "RGB", Imagick::PIXEL_CHAR);
    $binaryValue = pack('c*', ...$pixels);

    $packed = "";
    foreach ( $pixels as $byte ) {
        $packed .= chr( $byte );
    }
    header('Content-Type: application/octet-stream');
    echo $packed;
  } else {
    echo 'BNo URL specifeid.';
}
?>