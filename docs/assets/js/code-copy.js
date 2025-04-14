document.addEventListener( "DOMContentLoaded", function() {
    document.querySelectorAll( "div.highlight" ).forEach( ( block ) => {
        const button = document.createElement( "button" );
        button.textContent = "Copy";
        button.className = "btn btn-primary copy-btn";
        button.addEventListener( "click", async () => {
            const code = block.querySelector( "pre" ).innerText;
            try {
                if ( navigator.clipboard && window.isSecureContext ) {
                    // For HTTPS or localhost


                    await navigator.clipboard.writeText( code );
                    button.textContent = "Copied!";
                }
                else {
                    // Fallback for HTTP
                    const textArea = document.createElement( "textarea" );
                    textArea.value = code;
                    textArea.style.position = "fixed";
                    textArea.style.left = "-999999px";
                    textArea.style.top = "-999999px";
                    document.body.appendChild( textArea );
                    textArea.focus();
                    textArea.select();
                    try {
                        document.execCommand( "copy" );
                        button.textContent = "Copied!";
                    }
                    catch ( err ) {
                        button.textContent = "Failed to copy";
                    }
                    textArea.remove();
                }
            }
            catch ( err ) {
                button.textContent = "Failed to copy";
            }
            setTimeout( () => ( button.textContent = "Copy" ), 2000 );
        } );
        block.appendChild( button );
    } );
} );
