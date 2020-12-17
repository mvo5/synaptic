Synaptic
========

Synaptic is a graphical package management tool based on GTK+ and APT.
Synaptic enables you to install, upgrade and remove software packages in
a user friendly way.

Besides these basic functions the following features are provided:
 * Search and filter the list of available packages
 * Perform smart system upgrades
 * Fix broken package dependencies
 * Edit the list of used repositories (sources.list)
 * Download the latest changelog of a package
 * Configure packages through the debconf system
 * Browse all available documentation related to a package (dwww is required)

Synaptic was developed by Alfredo K. Kojima from Connectiva. I completed his port to GTK and added some new features. Connectiva continuted to support the project with contributions from Gustavo Niemeyer for some time after that.

See the NEWS file for modern user visible changes.
See Contributing.md for how to contribute or get in touch.

Usage
---------
By default synaptic uses pkexec to obtain root privileges needed.

Synaptic is used very much like apt/apt-get. Aside from a graphical interface, another key difference is it let's you stage several changes and only applies package changes when you click apply.

A simple upgrade
```
sudo apt update
sudo apt upgrade
Review changes and press yes
```

Do an upgrade in synaptic:
 1. Click Reload
 2. Note how Installed (upgradeable) appears in the Filter list
 3. Click Mark All Upgrades
 4. Mark additional Required Changes may appear
 5. Click Mark
 6. Click Apply
 7. Summary appears, Click Apply.

Filters
--------
Synaptic display the main package list according to the filter you selected. The most simple filter is of course "All packages". But there are much more filters than that :) You can view the predefined filters and make your own filters by clicking on "Filters" above the main package list.

Selecting Multiple Packages
----------------------------
You can select more than one package at a time. You have to use Shift or Ctrl to select multiple packages. If you click on a action (install/upgrade/remove) for multiple packages, the action will be performed for each package (as you probably already guessed (: ).

Multiple Sources
----------------
On a Debian system, you can have more than one "release" in your sources.list file. You can choose which one to use in the "Distribution" tab in the preferences dialog.

Keybindings
------------
Global keybings:
* Alt-K  keep
* Alt-I  install
* Alt-R  remove
* Alt-U  Update individual package
* Alt-L  Update Package List
* Alt-G  upgrade
* Alt-D  DistUpgrade
* Alt-P  proceed
* Ctrl-F find

Command line options
---------------------
Synaptic supports the following command-line options:
 '-f <filename>' or "--filter-file <filename>" = give a alternative filter file
 '-i <int>' or "--initial-filter <int>" = start with filter nr. <int>
 '-r' = open repository screen on startup
 '-o <option>' or "--option <option>" = set a synaptic/apt option (expert only)
 '--set-selections' = feed packages inside synaptic (format is like
                      dpkg --get-selections)
 '--non-interactive' = non-interactive mode (this will also prevent saving 
                       of configuration options)

Have fun with synaptic,
 Michael Vogt
