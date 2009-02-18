
DESCRIPTION="A small 9P protocol library"
HOMEPAGE="http://j.metux.de/index.php?option=com_content&task=view&id=51"
SRC_URI="http://releases.metux.de/${PN}/${PN}-${PV}.tar.bz2"

LICENSE="LGPL"
SLOT="0"
KEYWORDS="~alpha ~amd64 ~hppa ~ia64 ~mips ~ppc ~ppc64 ~s390 ~sparc x86"
IUSE=""

DEPEND="sys-libs/stdc-pkgconfig"

src_compile() {
	emake
}

src_install() {
	emake DESTDIR="${D}" install || die
	dodoc AUTHORS ChangeLog README
}
