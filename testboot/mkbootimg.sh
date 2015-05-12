PROJECT=${1:-krillin}
abootimg --create boot.img -k ../out/target/product/$PROJECT/kernel_$PROJECT.bin -r initrd.img -f bootimg.cfg
