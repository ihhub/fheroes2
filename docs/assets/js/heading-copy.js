document.addEventListener('DOMContentLoaded', () => {
    const content = document.querySelector('#main_content');
    if (!content) return;

    // Constants
    const LINK_ICON = '🔗';
    const COPIED_ICON = '&nbsp;- Copied';
    const ERROR_ICON = '❌';
    const RESET_DELAY = 2000;
    const HEADING_SELECTORS = 'h2, h3, h4, h5, h6';

    // Create a persistent live region for announcements
    const createLiveRegion = () => {
        const liveRegion = document.createElement('div');
        liveRegion.setAttribute('aria-live', 'assertive');
        liveRegion.setAttribute('aria-atomic', 'true');
        liveRegion.style.position = 'absolute';
        liveRegion.style.width = '1px';
        liveRegion.style.height = '1px';
        liveRegion.style.padding = '0';
        liveRegion.style.margin = '-1px';
        liveRegion.style.overflow = 'hidden';
        liveRegion.style.clip = 'rect(0, 0, 0, 0)';
        liveRegion.style.whiteSpace = 'nowrap';
        liveRegion.style.border = '0';
        document.body.appendChild(liveRegion);
        return liveRegion;
    };

    // Create a single persistent live region for all announcements
    const liveRegion = createLiveRegion();

    /**
     * Creates a unique ID for a heading based on its text content
     * @param {string} text - The heading text
     * @returns {string} - A unique ID
     */
    const generateHeadingId = (text) => {
        return text.toLowerCase().replace(/[^a-z0-9]+/g, '-');
    };

    /**
     * Creates a link icon element with proper accessibility attributes
     * @returns {HTMLElement} - The icon element
     */
    const createIconElement = () => {
        const icon = document.createElement('span');
        icon.className = 'heading-anchor';
        icon.setAttribute('aria-label', 'Copy link to heading');
        icon.setAttribute('role', 'button');
        icon.setAttribute('tabindex', '0');
        icon.innerHTML = LINK_ICON;
        return icon;
    };

    /**
     * Creates a heading link with proper accessibility attributes
     * @param {HTMLElement} heading - The heading element
     */
    const createHeadingLink = (heading) => {
        // Generate ID if needed
        if (!heading.id) {
            heading.id = generateHeadingId(heading.textContent);
        }

        // Create icon element
        const icon = createIconElement();

        // Add icon to the heading
        heading.appendChild(icon);

        // Handle click and keyboard events
        const handleCopy = async (e) => {
            e.preventDefault();
            e.stopPropagation();

            const url = new URL(window.location.href);
            url.hash = heading.id;

            try {
                await navigator.clipboard.writeText(url.toString());

                // Show confirmation
                icon.innerHTML = COPIED_ICON;
                icon.style.transform = 'scale(1.2)';

                // Announce to screen readers using the persistent live region
                liveRegion.textContent = `Link to ${heading.textContent} copied to clipboard`;

                // Force a reflow to ensure the announcement is triggered
                void liveRegion.offsetHeight;

                // Update URL without scrolling
                history.pushState(null, null, `#${heading.id}`);

                // Reset icon after delay
                setTimeout(() => {
                    icon.innerHTML = LINK_ICON;
                    icon.style.transform = '';
                }, RESET_DELAY);
            } catch (err) {
                console.error('Failed to copy:', err);
                icon.innerHTML = ERROR_ICON;

                // Announce error to screen readers using the persistent live region
                liveRegion.textContent = `Failed to copy link to ${heading.textContent}`;

                // Force a reflow to ensure the announcement is triggered
                void liveRegion.offsetHeight;

                setTimeout(() => {
                    icon.innerHTML = LINK_ICON;
                }, RESET_DELAY);
            }
        };

        // Add event listeners for both click and keyboard
        icon.addEventListener('click', handleCopy);
        icon.addEventListener('keydown', (e) => {
            if (e.key === 'Enter' || e.key === ' ') {
                handleCopy(e);
            }
        });
    };

    // Apply to all headings
    content.querySelectorAll(HEADING_SELECTORS).forEach(createHeadingLink);
});
