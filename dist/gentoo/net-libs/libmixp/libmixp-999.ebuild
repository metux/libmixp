
DESCRIPTION="A small 9P protocol library [trunk]"
HOMEPAGE="http://j.metux.de/index.php?option=com_content&task=view&id=51"
SRC_URI="http://releases.metux.de/${PN}/${PN}-${PV}.tar.bz2"

LICENSE="LGPL"
SLOT="0"
KEYWORDS="~alpha ~amd64 ~hppa ~ia64 ~mips ~ppc ~ppc64 ~s390 ~sparc ~x86"
IUSE=""

DEPEND="sys-libs/stdc-pkgconfig"

src_compile() {
	ewarn "This is the current trunk. You should NOT install it, unless you're actively developing !"
	cd libmixp
	emake
}

src_install() {
	ewarn "This is the current trunk. You should NOT install it, unless you're actively developing !"
	cd libmixp
	make clean
	emake DESTDIR="${D}" install || die
	dodoc AUTHORS ChangeLog README
}
