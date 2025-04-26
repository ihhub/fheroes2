/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// Constants
const LINK_ICON = '🔗';
const COPIED_ICON = ' - Copied';
const ERROR_ICON = '❌';
const RESET_DELAY = 2000;
const HEADING_SELECTORS = 'h2, h3, h4, h5, h6';

// Class to handle heading copy functionality
class HeadingCopyManager
{
    // Static instance of AccessibilityManager
    static accessibility = null;

    // Static initialization method
    static initialize()
    {
        // Initialize accessibility manager if not already initialized
        if ( !this.accessibility ) {
            this.accessibility = new window.fheroes2.AccessibilityManager();
        }

        // Apply to all headings
        document.querySelectorAll( HEADING_SELECTORS ).forEach( heading => this.createHeadingLink( heading ) );
    }

    // Creates a unique ID for a heading based on its text content
    // Parameters:
    //   text - The heading text
    // Returns: A unique ID
    static generateHeadingId( text )
    {
        return text.toLowerCase().replace( /[^a-z0-9]+/g, '-' );
    }

    // Creates a link icon element with proper accessibility attributes
    // Returns: The icon element
    static createIconElement()
    {
        const icon = this.accessibility.createAccessibleSpan( { text : LINK_ICON, ariaLabel : 'Copy link to heading', className : 'heading-anchor' } );
        return icon;
    }

    // Updates the icon appearance based on the copy result
    // Parameters:
    //   icon - The icon element
    //   success - Whether the copy was successful
    //   headingText - The text of the heading
    static updateIconState( icon, success, headingText )
    {
        this.accessibility.updateElementState( {
            element : icon,
            success : success,
            successMessage : `Link to ${headingText} copied to clipboard`,
            errorMessage : `Failed to copy link to ${headingText}`,
            successText : COPIED_ICON,
            errorText : ERROR_ICON,
            defaultText : LINK_ICON,
            resetDelay : RESET_DELAY,
            isHtml : true // Use innerHTML for the heading icon
        } );
    }

    // Handles the copy functionality for a heading
    // Parameters:
    //   e - The event object
    //   heading - The heading element
    //   icon - The icon element
    static handleCopy( e, heading, icon )
    {
        e.preventDefault();
        e.stopPropagation();

        const url = new URL( window.location.href );
        url.hash = heading.id;

        try {
            navigator.clipboard.writeText( url.toString() )
                .then( () => {
                    this.updateIconState( icon, true, heading.textContent );

                    // Update URL without scrolling
                    history.pushState( null, null, `#${heading.id}` );
                } )
                .catch( err => {
                    console.error( 'Failed to copy:', err );
                    this.updateIconState( icon, false, heading.textContent );
                } );
        }
        catch ( err ) {
            console.error( 'Failed to copy:', err );
            this.updateIconState( icon, false, heading.textContent );
        }
    }

    // Sets up event listeners for a heading icon
    // Parameters:
    //   icon - The icon element
    //   heading - The heading element
    static setupEventListeners( icon, heading )
    {
        // Add click event handler
        icon.addEventListener( 'click', ( e ) => this.handleCopy( e, heading, icon ) );

        // Add keyboard event handler
        this.accessibility.setupKeyboardEvents( icon, ( e ) => this.handleCopy( e, heading, icon ) );
    }

    // Creates a heading link with proper accessibility attributes
    // Parameters:
    //   heading - The heading element
    static createHeadingLink( heading )
    {
        // Generate ID if needed
        if ( !heading.id ) {
            heading.id = this.generateHeadingId( heading.textContent );
        }

        // Create icon element
        const icon = this.createIconElement();

        // Add icon to the heading
        heading.appendChild( icon );

        // Set up event listeners
        this.setupEventListeners( icon, heading );
    }
}

// Initialize when the DOM is fully loaded
document.addEventListener( 'DOMContentLoaded', () => { HeadingCopyManager.initialize(); } );
