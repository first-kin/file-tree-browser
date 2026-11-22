<script setup lang="ts">defineProps<{title:string;description?:string}>()</script><template><div class="page-heading"><div><h2>{{title}}</h2><p>{{description}}</p></div><slot/></div></template>
