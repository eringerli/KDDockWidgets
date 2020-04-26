pkgname=kddockwidgets-eringerli-git # '-bzr', '-git', '-hg' or '-svn'
pkgver=git
pkgrel=1
pkgdesc="KDAB's KDDockWidgets, eringerli's fork"
arch=('x86_64' 'armv7h' 'aarch64')
url="https://github.com/eringerli/KDDockWidgets"
license=('GPL')
groups=()
depends=('qt5-base' 'qt5-x11extras')
makedepends=('git' 'cmake')
provides=("${pkgname}")
conflicts=("${pkgname}")
replaces=()
backup=()
options=()
install=
source=('kddockwidgets-eringerli-git::git+https://github.com/eringerli/KDDockWidgets#branch=pkgbuild')
noextract=()
md5sums=('SKIP')

# Please refer to the 'USING VCS SOURCES' section of the PKGBUILD man page for
# a description of each element in the source array.

pkgver() {
  cd "$pkgname"
  ( set -o pipefail
    git describe --long 2>/dev/null | sed 's/\([^-]*-g\)/r\1/;s/-/./g' ||
    printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
  )
}

prepare() {
  cd "$pkgname"
}

build() {
  cd "$srcdir/${pkgname}"
  mkdir -p build
  cd build
  cmake -DCMAKE_INSTALL_PREFIX=/usr ..
  make
}

package() {
  cd "$srcdir/${pkgname}/build"
  make DESTDIR="$pkgdir/" install
}
