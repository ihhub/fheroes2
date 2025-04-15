document.addEventListener( "DOMContentLoaded", () => {
    // Create a dedicated live region for announcements
    const liveRegion = document.createElement( "div" );
    liveRegion.setAttribute( "aria-live", "assertive" );
    liveRegion.setAttribute( "aria-atomic", "true" );
    liveRegion.setAttribute( "role", "status" );
    liveRegion.style.position = "absolute";
    liveRegion.style.width = "1px";
    liveRegion.style.height = "1px";
    liveRegion.style.padding = "0";
    liveRegion.style.margin = "-1px";
    liveRegion.style.overflow = "hidden";
    liveRegion.style.clip = "rect(0, 0, 0, 0)";
    liveRegion.style.whiteSpace = "nowrap";
    liveRegion.style.border = "0";
    document.body.appendChild( liveRegion );

    document.querySelectorAll( "div.highlight" ).forEach( ( block ) => {
        // Create copy button
        const button = document.createElement( "button" );
        button.textContent = "Copy";
        button.className = "btn btn-primary copy-btn";

        // Enhanced accessibility attributes
        button.setAttribute( "aria-label", "Copy code to clipboard" );
        button.setAttribute( "title", "Copy code to clipboard" );
        button.setAttribute( "role", "button" );
        button.setAttribute( "tabindex", "0" );

        // Add button to the code block
        block.appendChild( button );

        // Handle copy functionality
        const copyCode = async () => {
            try {
                const code = block.querySelector( "pre" ).innerText;
                await navigator.clipboard.writeText( code );

                // Visual feedback
                button.textContent = "Copied!";
                button.classList.add( "success" );

                // Update the button's aria-label
                button.setAttribute( "aria-label", "Code copied to clipboard" );

                // Clear the live region first
                liveRegion.textContent = "";

                // Force a reflow
                void liveRegion.offsetHeight;

                // Set the new content after a tiny delay
                setTimeout( () => { liveRegion.textContent = "Code copied to clipboard"; }, 10 );

                // Reset button after delay
                setTimeout( () => {
                    button.textContent = "Copy";
                    button.classList.remove( "success" );
                    button.setAttribute( "aria-label", "Copy code to clipboard" );
                }, 2000 );
            }
            catch ( err ) {
                console.error( "Failed to copy code:", err );
                button.textContent = "Failed to copy";
                button.classList.add( "error" );

                // Update the button's aria-label
                button.setAttribute( "aria-label", "Failed to copy code to clipboard" );

                // Clear the live region first
                liveRegion.textContent = "";

                // Force a reflow
                void liveRegion.offsetHeight;

                // Set the new content after a tiny delay
                setTimeout( () => { liveRegion.textContent = "Failed to copy code to clipboard"; }, 10 );

                // Reset button after delay
                setTimeout( () => {
                    button.textContent = "Copy";
                    button.classList.remove( "error" );
                    button.setAttribute( "aria-label", "Copy code to clipboard" );
                }, 2000 );
            }
        };

        // Click event handler
        button.addEventListener( "click", copyCode );

        // Keyboard event handler for accessibility
        button.addEventListener( "keydown", ( event ) => {
            if ( event.key === "Enter" || event.key === " " ) {
                event.preventDefault();
                copyCode();
            }
        } );
    } );
} );
