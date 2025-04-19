# Website Local Documentation Development Guide

This guide explains how to set up and run the documentation site locally for
development purposes.

## Prerequisites

* Ruby (version 2.7.0 or higher)

* Bundler
* Git
* Node/npm
* A GitHub account with a fork of the fheroes2 repository

### macOS Users

If you're on macOS, you can install the required development tools using the
provided Brewfile:

```bash
# Install the development tools using brew bundle
brew bundle --file=script/macos/Brewfile.dev_tools
```

This will install the necessary tools to assist with formatting and for linting
source files.

## Setting Up Local Development Environment

1. Clone your fork of the repository:

   ```bash
   git clone https://github.com/<YOUR_USERNAME>/fheroes2.git
   cd fheroes2/docs
   ```

2. Copy the development Gemfile to enable local development:

   ```bash
   cp Gemfile.dev Gemfile
   ```

   > **Note**
   > While the gem in this file may eventually be deprecated, it's currently the
   > simplest way to enable local development of the GitHub Pages site. As such,
   > the Gemfile should not be committed into the repo.

3. Install the required Ruby dependencies:

   ```bash
   bundle install
   ```

4. Install the npm dev packages:

   ```bash
   npm install
   ```

5. Build the site locally: (Optional)

   ```bash
   bundle exec jekyll build
   ```

6. To preview the site locally with live reload:

   ```bash
   bundle exec jekyll serve
   ```

   This will start a local server, typically at `http://localhost:4000`

## Making Changes

1. Create a new branch for your documentation changes:

   ```bash
   git checkout -b docs/your-feature-name
   ```

2. Make your changes to the documentation files in the `docs` directory.

3. Test your changes locally using the serve command mentioned above.

4. Verify your markdown file changes using the project's linting tools:

   > **Note**
   > We use formatting/linting tools to ensure consistent formatting and quality
   > across all markdown files.

   ```bash
   cd docs
   # Run the linter on all Markdown files in the docs directory
   npm run lint-md "*.md"
   # Or have the linter attempt to format automatically
   npm run format-md "*.md"
   ```

   This will check your Markdown files against our style rules and highlight any
   issues that need to be fixed.

5. Format HTML, CSS, and JavaScript files using the provided npm scripts:

   ```bash
   # Format HTML and CSS files
   npm run format-html-css

   # Format JavaScript files
   npm run format-js
   ```

6. Commit your changes:

   ```bash
   git add .
   git commit -m "docs: your descriptive commit message"
   ```

7. Push your branch to your fork:

   ```bash
   git push origin docs/your-feature-name
   ```

## Verifying Site Changes in Fork

For the most accurate verification of your documentation changes, follow these
steps:

1. Push your branch to your fork:

   ```bash
   git push origin docs/your-feature-name
   ```

2. Set up GitHub Pages in your fork by following the
[official GitHub Pages documentation](https://docs.github.com/en/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site#publishing-from-a-branch).
Make sure to:

   * Configure the source to deploy from your feature branch
   * Set the folder to `/docs`

3. Wait for GitHub Pages to build and deploy your site. This may take a few
minutes. You can monitor the progress in the "Actions" tab of your repository.

4. Once deployed, you can preview your changes at `https://<YOUR_USERNAME>.github.io/fheroes2/`

5. Review your changes in the preview environment to ensure:

   * All links work correctly
   * Images are displayed properly
   * The layout is consistent with the main site
   * No broken references or missing assets

6. After verifying everything looks correct in your fork's GitHub Pages, create
a Pull Request (PR) from your fork's branch to the main fheroes2 repository for
final review and deployment.

## Common Issues and Solutions

* If you encounter Ruby version conflicts, make sure you're using a compatible
version. You can use tools like `rbenv` or `rvm` to manage Ruby versions.
* If the site doesn't build, check the Jekyll build logs for specific error
messages.
* If images aren't displaying, ensure they're placed in the correct directory
(`docs/images/`) and referenced correctly in your markdown files.

## Additional Resources

* [Jekyll Documentation](https://jekyllrb.com/docs/)
* [GitHub Pages Documentation](https://docs.github.com/en/pages)
* [GitHub Pages Quickstart Guide](https://docs.github.com/en/pages/quickstart#introduction)
* [Deploying from a branch in GitHub Pages](https://docs.github.com/en/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site#choosing-a-publishing-source)
* [Markdown Guide](https://www.markdownguide.org/)
