<?php

if (isset($_GET['url'])) {
    $imageUrl = $_GET['url'];
   
    // volumio sends the the albumart URLs either as a full quylified URL or as
    // a relative volumio based URL
    // format the given URL as needed 
    if (substr_compare($imageUrl, "http", 0, 4) === 0 ) {  
        // for any reasons my php-setup does not allow to use https... simple hack
        // until I find out how to make this possible
        if (substr_compare($imageUrl, "https", 0, 5) === 0 ) {
            $remote_image = file_get_contents( "http" .substr($imageUrl,5 ));
        } else {
            $remote_image = file_get_contents( $imageUrl);
        }
    } else {
        // Please change this base volumio URL to your address
        $remote_image = file_get_contents("http://volumio.local" . $imageUrl);
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